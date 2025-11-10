#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <fcntl.h>
#include <sys/mman.h>

#include "r500/3d_registers.h"
#include "r500/3d_registers_undocumented.h"
#include "r500/3d_registers_bits.h"
#include "r500/indirect_buffer.h"
#include "r500/shader.h"
#include "r500/display_controller.h"

#include "drm/buffer.h"
#include "drm/drm.h"

#include "math/float_types.hpp"
#include "math/transform.hpp"
#include "math/constants.hpp"

#include "../model/model2.h"

#define CLEAR_SHADER 0
#define PLANE_SHADER 1
#define PARTICLE_SHADER 2
#define TEXTURE_TILE_SHADER 3

const char * vertex_shader_paths[] = {
  "clear.vs.bin",
  "particle_plane_fan.vs.bin",
  "particle_particle_animated_quad_vbuf.vs.bin",
  "texture_tile.vs.bin",
};
const int vertex_shader_paths_length = (sizeof (vertex_shader_paths)) / (sizeof (vertex_shader_paths[0]));
const char * fragment_shader_paths[] = {
  "clear.fs.bin",
  "particle_plane.fs.bin",
  "particle_particle.fs.bin",
  "texture_tile.fs.bin",
};
const int fragment_shader_paths_length = (sizeof (fragment_shader_paths)) / (sizeof (fragment_shader_paths[0]));

#define PLANE_TEXTURE 0
#define PARTICLE_TEXTURE 1

const char * textures[] = {
  "../texture/plane_32x32_rgba8888.data",
  "../texture/particle_32x32_rgba8888.data",
};
const int textures_length = (sizeof (textures)) / (sizeof (textures[0]));

struct shaders {
  struct shader_offset * vertex;
  struct shader_offset * fragment;
  int vertex_length;
  int fragment_length;
};

static inline uint32_t xorshift32(uint32_t state)
{
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

static inline float xorshift32f(uint32_t& state)
{
  state = xorshift32(state);
  return (float)(state & 0xffffff) * (1.0f / 16777215.0f);
}

const float max_age = 3.0f;

struct particle {
  vec3 position;
  float time;
  float delta;
  vec3 velocity;
};

void _3d_clear(const shaders& shaders)
{
  ib_rs_instructions(0);

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , 0);

  //

  ib_zbuffer(ZBUFFER_RELOC_INDEX, 1600, 7); // always

  ib_texture__0();

  ib_vap_stream_cntl__2();

  // shaders
  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(1)
      );
  ib_ga_us(&shaders.fragment[CLEAR_SHADER]);
  ib_vap_pvs(&shaders.vertex[CLEAR_SHADER]);

  //////////////////////////////////////////////////////////////////////////////
  // VAP INDEX
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_INDEX_OFFSET, 0);

  T0V(VAP_VF_MAX_VTX_INDX
      , VAP_VF_MAX_VTX_INDX__MAX_INDX(0)
      );
  T0V(VAP_VF_MIN_VTX_INDX
      , VAP_VF_MIN_VTX_INDX__MIN_INDX(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__CLIP_DISABLE(1)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VTX_XY_FMT(1) // disable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(1)  // disable W division
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // GA POINT SIZE
  //////////////////////////////////////////////////////////////////////////////

  T0V(GA_POINT_SIZE
      , GA_POINT_SIZE__HEIGHT(600 * 12)
      | GA_POINT_SIZE__WIDTH(800 * 12)
      );

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const int dwords_per_vtx = 2;

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(dwords_per_vtx)
      );

  const float center[] = {
    800.0f, 600.0f,
  };
  const int vertex_count = 1;
  T3(_3D_DRAW_IMMD_2, (1 + vertex_count * dwords_per_vtx) - 1);
  TU( VAP_VF_CNTL__PRIM_TYPE(1) // point list
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );
  for (int i = 0; i < 2; i++) {
    TF(center[i]);
  }
}

mat4x4 perspective(float low1, float high1,
                   float low2, float high2,
                   float low3, float high3)
{
  float scale2 = (high2 - low2) / (high1 - low1);
  float scale3 = (high3 - low3) / (high1 - low1);

  mat4x4 m1 = mat4x4(1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 1, -low1,
                     0, 0, 0, 1
                     );

  mat4x4 m2 = mat4x4(1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, scale2, low2,
                     0, 0, scale3, low3
                     );

  return m2 * m1;
}

void _3d_plane_inner()
{
  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const int dwords_per_vtx = 2;

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(dwords_per_vtx)
      );

  const vec2 vertices[] = {
    {0.0, 0.0f},
    {1.0, 0.0f},
    {1.0, 1.0f},
    {0.0, 1.0f},
  };

  const int vertex_count = 4;
  T3(_3D_DRAW_IMMD_2, (1 + vertex_count * dwords_per_vtx) - 1);
  TU( VAP_VF_CNTL__PRIM_TYPE(5) // triangle fan
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );
  for (int i = 0; i < vertex_count; i++) {
    TF(vertices[i].x);
    TF(vertices[i].y);
  }
}

void _3d_particle_inner(int particles_length, int position_offset)
{
  const int vertex_count = 4 * particles_length;

  //////////////////////////////////////////////////////////////////////////////
  // VF
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_VF_MAX_VTX_INDX
      , VAP_VF_MAX_VTX_INDX__MAX_INDX(vertex_count - 1)
      );
  T0V(VAP_VF_MIN_VTX_INDX
      , VAP_VF_MIN_VTX_INDX__MIN_INDX(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // AOS
  //////////////////////////////////////////////////////////////////////////////

  T3(_3D_LOAD_VBPNTR, (4 - 1));
  TU( // VAP_VTX_NUM_ARRAYS
      VAP_VTX_NUM_ARRAYS__VTX_NUM_ARRAYS(2)
    | VAP_VTX_NUM_ARRAYS__VC_FORCE_PREFETCH(1)
    );
  TU( // VAP_VTX_AOS_ATTR01
      VAP_VTX_AOS_ATTR__VTX_AOS_COUNT0(3)
    | VAP_VTX_AOS_ATTR__VTX_AOS_STRIDE0(3)
    | VAP_VTX_AOS_ATTR__VTX_AOS_COUNT1(2)
    | VAP_VTX_AOS_ATTR__VTX_AOS_STRIDE1(2)
    );
  TU( // VAP_VTX_AOS_ADDR0
      (4 * position_offset);
    );
  TU( // VAP_VTX_AOS_ADDR1
      (4 * 0);
    );

  T3(_NOP, 0);
  TU(VERTEXBUFFER_RELOC_INDEX * 4); // index into relocs array for VAP_VTX_AOS_ADDR0
  T3(_NOP, 0);
  TU(VERTEXBUFFER_RELOC_INDEX * 4); // index into relocs array for VAP_VTX_AOS_ADDR0

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  T3(_3D_DRAW_VBUF_2, (1 - 1));
  TU( VAP_VF_CNTL__PRIM_TYPE(13) // quad list
    | VAP_VF_CNTL__PRIM_WALK(2) // vertex list (data fetched from memory)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );
}

void _3d_plane(const shaders& shaders,
               const mat4x4& world_to_clip,
               float theta)
{
  ib_rs_instructions(1);

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , VAP_OUT_VTX_FMT_1__TEX_0_COMP_CNT(4));

  //

  ib_zbuffer(ZBUFFER_RELOC_INDEX, 1600, 1); // less

  int width = 32;
  int height = 32;
  int macrotile = 0;
  int microtile = 0;
  int clamp = 0; // wrap/repeat
  ib_texture__1(TEXTUREBUFFER_RELOC_INDEX + PLANE_TEXTURE,
                width, height,
                macrotile, microtile,
                clamp);

  ib_vap_stream_cntl__2();

  // shaders
  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(4)
      );
  ib_ga_us(&shaders.fragment[PLANE_SHADER]);
  ib_vap_pvs(&shaders.vertex[PLANE_SHADER]);

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__PS_UCP_MODE(3)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VPORT_X_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_X_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VPORT_Z_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Z_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VTX_XY_FMT(0) // enable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(0)  // enable W division
      | VAP_VTE_CNTL__VTX_W0_FMT(1)
      | VAP_VTE_CNTL__SERIAL_PROC_ENA(0)
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // matrix
  //////////////////////////////////////////////////////////////////////////////

  mat4x4 s = scale(1.0f);
  mat4x4 rx = rotate_x(-PI / 2.0f);
  mat4x4 local_to_world = s * rx;

  mat4x4 trans = world_to_clip * local_to_world;

  //////////////////////////////////////////////////////////////////////////////
  // consts
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);

  const float consts[] = {
    // 0
    trans[0][0], trans[0][1], trans[0][2], trans[0][3],
    trans[1][0], trans[1][1], trans[1][2], trans[1][3],
    trans[2][0], trans[2][1], trans[2][2], trans[2][3],
    trans[3][0], trans[3][1], trans[3][2], trans[3][3],
    // 4
    -2.0f, 0, 0, 0,
  };
  ib_vap_pvs_const_cntl(consts, (sizeof (consts)));

  // plane_inner

  _3d_plane_inner();
}

void _3d_particle(const shaders& shaders,
                  const mat4x4& world_to_clip,
                  const mat4x4& world_to_view,
                  const particle * particles,
                  const int particles_length,
                  const float theta,
                  float * vertexbuffer_ptr)
{
  // enable blending
  T0V(RB3D_BLENDCNTL
      , RB3D_BLENDCNTL__ALPHA_BLEND_ENABLE__ENABLE
      | RB3D_BLENDCNTL__READ_ENABLE(1)
      | RB3D_BLENDCNTL__SRCBLEND__GL_ONE
      | RB3D_BLENDCNTL__DESTBLEND__GL_ONE
      | RB3D_BLENDCNTL__SRC_ALPHA_0_NO_READ(0)
      | RB3D_BLENDCNTL__SRC_ALPHA_1_NO_READ(0)
      );

  ib_rs_instructions(1);

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , VAP_OUT_VTX_FMT_1__TEX_0_COMP_CNT(4));

  //

  //ib_zbuffer(ZBUFFER_RELOC_INDEX, 1600, 1); // less
  T0V(ZB_CNTL
      , 0
      );
  T0V(ZB_ZSTENCILCNTL
      , 0
      );

  int width = 32;
  int height = 32;
  int macrotile = 0;
  int microtile = 0;
  int clamp = 0; // wrap/repeat
  ib_texture__1(TEXTUREBUFFER_RELOC_INDEX + PARTICLE_TEXTURE,
                width, height,
                macrotile, microtile,
                clamp);

  ib_vap_stream_cntl__32();

  // shaders
  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(4)
      );
  ib_ga_us(&shaders.fragment[PARTICLE_SHADER]);
  ib_vap_pvs(&shaders.vertex[PARTICLE_SHADER]);

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__PS_UCP_MODE(3)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VPORT_X_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_X_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VPORT_Z_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Z_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VTX_XY_FMT(0) // enable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(0)  // enable W division
      | VAP_VTE_CNTL__VTX_W0_FMT(1)
      | VAP_VTE_CNTL__SERIAL_PROC_ENA(0)
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // matrix
  //////////////////////////////////////////////////////////////////////////////

  mat4x4 s = scale(1.0f);
  mat4x4 local_to_world = s;

  mat4x4 local_to_view = world_to_view * local_to_world;

  mat4x4 trans = world_to_clip * local_to_world;

  //////////////////////////////////////////////////////////////////////////////
  // consts
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);

  const float scale = 0.005f;

  const float consts[] = {
    // 0
    trans[0][0], trans[0][1], trans[0][2], trans[0][3],
    trans[1][0], trans[1][1], trans[1][2], trans[1][3],
    trans[2][0], trans[2][1], trans[2][2], trans[2][3],
    trans[3][0], trans[3][1], trans[3][2], trans[3][3],

    // 4: dx (right)
    local_to_view[0][0], local_to_view[0][1], local_to_view[0][2], 0,

    // 5: dy (up)
    local_to_view[1][0], local_to_view[1][1], local_to_view[1][2], 0,

    // 6: xyz:position w:scale
    0, 0, 0, scale,

    // 7:
    -2.0, 0, 0, 0,
  };
  ib_vap_pvs_const_cntl(consts, (sizeof (consts)));

  int offset = particles_length * 4 * 2;
  int ix = 0;
  for (int i = 0; i < particles_length; i++) {
    const vec3& position = particles[i].position;
    for (int j = 0; j < 4; j++) {
      vertexbuffer_ptr[offset + ix] = position.x;
      ix++;
      vertexbuffer_ptr[offset + ix] = position.y;
      ix++;
      vertexbuffer_ptr[offset + ix] = position.z;
      ix++;
    };
  }
  asm volatile ("" ::: "memory");

  _3d_particle_inner(particles_length, offset);
}

int indirect_buffer(const shaders& shaders,
                    const particle * particles,
                    const int particles_length,
                    float theta,
                    float * vertexbuffer_ptr)
{
  int width = 1600;
  int height = 1200;
  int pitch = width;

  ib_ix = 0;

  ib_generic_initialization();

  T0V(RB3D_BLENDCNTL, 0);
  T0V(RB3D_ABLENDCNTL, 0);

  ib_viewport(width, height);
  ib_colorbuffer(COLORBUFFER_RELOC_INDEX, pitch, 0, 0);

  T0V(GB_ENABLE, 0);

  T0V(US_OUT_FMT_0
      , US_OUT_FMT__OUT_FMT(0)  // C4_8
      | US_OUT_FMT__C0_SEL__BLUE
      | US_OUT_FMT__C1_SEL__GREEN
      | US_OUT_FMT__C2_SEL__RED
      | US_OUT_FMT__C3_SEL__ALPHA
      | US_OUT_FMT__OUT_SIGN(0)
      );
  T0V(US_OUT_FMT_1
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_2
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_2
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );

  load_pvs_shaders(shaders.vertex, shaders.vertex_length);
  load_us_shaders(shaders.fragment, shaders.fragment_length);

  //////////////////////////////////////////////////////////////////////////////
  // DRAW
  //////////////////////////////////////////////////////////////////////////////

  mat4x4 aspect = scale(vec3(3.0f/4.0f, 1, 1));
  mat4x4 p = perspective(0.01f, 3.0f,
                         0.001f, 0.999f,
                         1.0f, 3.0f);
  mat4x4 t = translate(vec3(0, 0, 1));
  mat4x4 rx = rotate_x(-PI / 8.0f);
  mat4x4 ry = rotate_y(theta * 0.8f);
  mat4x4 world_to_view = t * rx * ry;

  mat4x4 world_to_clip = aspect * p * world_to_view;

  _3d_clear(shaders);
  _3d_plane(shaders, world_to_clip, theta);
  _3d_particle(shaders,
               world_to_clip,
               world_to_view,
               particles,
               particles_length,
               theta,
               vertexbuffer_ptr);

  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ib_ix % 8) != 0) {
    TU(0x80000000);
  }

  assert((unsigned int)ib_ix < (sizeof (ib)) / (sizeof (ib[0])));

  return ib_ix;
}

void reset_particle(particle& p)
{
  //vec3 pos = normalize(p.position);

  p.position = normalize(vec3(p.velocity.x,
                              0,
                              p.velocity.z)) * 20.0f;

  //printf("position %f %f %f\n", p.position.x, p.position.y, p.position.z);

  p.velocity = vec3(p.velocity.x,
                    2.0f * p.delta,
                    p.velocity.z);

  //printf("velocity %f %f %f\n\n", p.velocity.x, p.velocity.y, p.velocity.z);
}

void init_particles(particle * particles, const int particles_length)
{
  uint32_t state = 0x12345678;

  const float rl = 1.0f / (float)(particles_length);

  for (int i = 0; i < particles_length; i++) {
    float fi = ((float)i);

    float sx = xorshift32f(state) * 2.0f - 1.0f;
    float sy = xorshift32f(state) * 2.0f - 1.0f;
    float sz = xorshift32f(state) * 2.0f - 1.0f;

    float delta = xorshift32f(state) * 0.5f + 0.5f;

    float vx = xorshift32f(state) * 2.0f - 1.0f;
    float vz = xorshift32f(state) * 2.0f - 1.0f;

    particles[i].time = max_age * sinf(fi * rl * 2) * 0.5f + 0.5f;
    particles[i].delta = delta;
    particles[i].position.x = sx;
    particles[i].position.y = sy;
    particles[i].position.z = sz;
    particles[i].velocity = normalize(vec3(vx * 0.5f, 0.0f, vz * 0.5f));
  }
}

int init_particles_vertexbuffer(int fd, int particles_length, float ** ptr_out)
{
  const vec2 vertices[] = {
    {0.0, 0.0f},
    {1.0, 0.0f},
    {1.0, 1.0f},
    {0.0, 1.0f},
  };
  const int vertex_count = 4;

  const int size = particles_length * vertex_count * 2 * (sizeof (float))
                 + particles_length * vertex_count * 3 * (sizeof (float));
  printf("%d size %d\n", particles_length, size);

  void * ptr;
  int handle = create_buffer(fd, size, &ptr);

  float * ptrf = (float*)ptr;

  int ix = 0;
  for (int j = 0; j < particles_length; j++) {
    for (int i = 0; i < vertex_count; i++) {
      ptrf[ix++] = vertices[i].x;
      ptrf[ix++] = vertices[i].y;
    }
  }
  printf("init vertexbuffer %d %d\n", ix, size);

  assert(ptr_out != NULL);
  *ptr_out = ptrf;

  return handle;
}

int main()
{
  struct shaders shaders = {
    .vertex = load_shaders(vertex_shader_paths, vertex_shader_paths_length),
    .fragment = load_shaders(fragment_shader_paths, fragment_shader_paths_length),
    .vertex_length = vertex_shader_paths_length,
    .fragment_length = fragment_shader_paths_length,
  };

  void * rmmio = map_pci_resource2();

  int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  assert(fd != -1);

  const int colorbuffer_size = 1600 * 1200 * 4;
  int colorbuffer_handle[2];
  int zbuffer_handle;
  int * texturebuffer_handle;
  //int flush_handle;
  int vertexbuffer_handle;

  void * colorbuffer_ptr[2];
  void * zbuffer_ptr;
  float * vertexbuffer_ptr;

  // colorbuffer
  colorbuffer_handle[0] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[0]);
  colorbuffer_handle[1] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[1]);
  zbuffer_handle = create_buffer(fd, colorbuffer_size, &zbuffer_ptr);
  //flush_handle = create_flush_buffer(fd);
  texturebuffer_handle = load_textures(fd, textures, textures_length);

  fprintf(stderr, "colorbuffer handle[0] %d\n", colorbuffer_handle[0]);
  fprintf(stderr, "colorbuffer handle[1] %d\n", colorbuffer_handle[1]);
  fprintf(stderr, "zbuffer handle %d\n", zbuffer_handle);

  int colorbuffer_ix = 0;
  float theta = PI * 0.5;

  particle particles[16 * 16] = {};
  const int particles_length = (sizeof (particles)) / (sizeof (particles[0]));
  vertexbuffer_handle = init_particles_vertexbuffer(fd, particles_length, &vertexbuffer_ptr);
  init_particles(particles, particles_length);
  fprintf(stderr, "vertexbuffer handle %d\n", vertexbuffer_handle);

  while (true) {
    int ib_dwords = indirect_buffer(shaders,
                                    particles,
                                    particles_length,
                                    theta,
                                    vertexbuffer_ptr);

    int ret = drm_radeon_cs(fd,
                            colorbuffer_handle[colorbuffer_ix],
                            zbuffer_handle,
                            vertexbuffer_handle,
                            texturebuffer_handle,
                            textures_length,
                            ib_dwords);
    if (ret == -1)
      break;

    primary_surface_address(rmmio, colorbuffer_ix);

    // next state
    theta += 0.01f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;

    //
    // update particles
    //
    for (int i = 0; i < particles_length; i++) {
      if (particles[i].time <= 0) {
        particles[i].time += max_age;
        reset_particle(particles[i]);
      } else {
        particles[i].time -= 0.01f;
        particles[i].position += vec3(particles[i].velocity.x * 0.9f,
                                      particles[i].velocity.y * 5.0f,
                                      particles[i].velocity.z * 0.9f);
        particles[i].velocity += vec3(0, -0.04, 0);
        if (particles[i].position.y < 0) {
          particles[i].position.y = fabsf(particles[i].position.y);
          particles[i].velocity.y *= -0.6f;
        }
      }
    }
  }

  {
    printf("colorbuffer0.data\n");
    int out_fd = open("colorbuffer0.data", O_RDWR|O_CREAT, 0644);
    assert(out_fd >= 0);
    ssize_t write_length = write(out_fd, colorbuffer_ptr[0], colorbuffer_size);
    assert(write_length == colorbuffer_size);
    close(out_fd);
  }
  {
    printf("zbuffer.data\n");
    int out_fd = open("zbuffer.data", O_RDWR|O_CREAT, 0644);
    assert(out_fd >= 0);
    ssize_t write_length = write(out_fd, zbuffer_ptr, colorbuffer_size);
    assert(write_length == colorbuffer_size);
    close(out_fd);
  }

  close(fd);
}
