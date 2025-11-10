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
#define PARTICLE_PHYSICS_SHADER 4
#define VERTEX_BUFFER_COPY_SHADER 5

#define PARTICLE_POSITION_RELOC_INDEX 5

const char * vertex_shader_paths[] = {
  "clear.vs.bin",
  "particle_plane_fan.vs.bin",
  "particle_particle_animated_quad_vbuf.vs.bin",
  "texture_tile.vs.bin",
  "particle_physics.vs.bin",
  "vertex_buffer_copy.vs.bin",
};
const int vertex_shader_paths_length = (sizeof (vertex_shader_paths)) / (sizeof (vertex_shader_paths[0]));
const char * fragment_shader_paths[] = {
  "clear.fs.bin",
  "particle_plane.fs.bin",
  "particle_particle.fs.bin",
  "texture_tile.fs.bin",
  "particle_physics.fs.bin",
  "vertex_buffer_copy.fs.bin",
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

struct particle_position {
  vec3 position;
  float age;
};

struct particle_velocity {
  vec3 velocity;
  float delta;
};

struct particle {
  vec3 position;
  float age;
  vec3 velocity;
  float delta;
};

struct floatbuffer_state {
  int handles[4];
  void * ptrs[4];
  int length;
  int flip;

  inline particle_position * position_output() const
  {
    int fb_output = (!flip) * 2;
    particle_position * out_pos = (particle_position *)this->ptrs[fb_output + 0];
    return out_pos;
  }
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
      VAP_VTX_AOS_ATTR__VTX_AOS_COUNT0(4)
    | VAP_VTX_AOS_ATTR__VTX_AOS_STRIDE0(4)
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
                  const floatbuffer_state& state,
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

  ib_vap_stream_cntl__42();

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

  int offset = state.length * 4 * 2;
  /*
  int ix = 0;
  particle_position * pos = state.position_output();
  for (int i = 0; i < state.length; i++) {
    const vec3& position = pos[i].position;
    for (int j = 0; j < 4; j++) {
      vertexbuffer_ptr[offset + ix] = position.x;
      ix++;
      vertexbuffer_ptr[offset + ix] = position.y;
      ix++;
      vertexbuffer_ptr[offset + ix] = position.z;
      ix++;
      vertexbuffer_ptr[offset + ix] = 1; // W
      ix++;
    };
  }
  asm volatile ("" ::: "memory");
  */

  _3d_particle_inner(state.length, offset);
}

void _copy_to_vertexbuffer(const shaders& shaders,
                           const floatbuffer_state& state,
                           int floatbuffer_width,
                           int floatbuffer_height)
{
  assert(floatbuffer_width <= 1024);
  int viewport_width = floatbuffer_width * 4;
  int viewport_height = floatbuffer_height;
  int texture_width = floatbuffer_width;
  int texture_height = floatbuffer_height;

  int macrotile = 0;
  int microtile = 0;

  T0V(SC_SCISSOR0
      , SC_SCISSOR0__XS0(0)
      | SC_SCISSOR0__YS0(0)
      );
  T0V(SC_SCISSOR1
      , SC_SCISSOR1__XS1(viewport_width - 1)
      | SC_SCISSOR1__YS1(viewport_height - 1)
      );
  T0Vf(VAP_VPORT_XSCALE, (float)viewport_width);
  T0Vf(VAP_VPORT_YSCALE, (float)viewport_height);

  int colorformat = 7; // ARGB32323232

  int offset = state.length * 4 * 2 * (sizeof (float));

  ib_colorbuffer3(0,
                  VERTEXBUFFER_RELOC_INDEX,
                  offset,
                  viewport_width,
                  macrotile,
                  microtile,
                  colorformat);

  T0V(US_OUT_FMT_0
      , US_OUT_FMT__OUT_FMT(21)  // C4_32_FP
      | US_OUT_FMT__C0_SEL__RED
      | US_OUT_FMT__C1_SEL__GREEN
      | US_OUT_FMT__C2_SEL__BLUE
      | US_OUT_FMT__C3_SEL__ALPHA
      | US_OUT_FMT__OUT_SIGN(0)
      );
  T0V(US_OUT_FMT_1
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_2
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_3
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );

  // shaders
  //load_pvs_shaders(shaders.vertex, shaders.vertex_length);
  //load_us_shaders(shaders.fragment, shaders.fragment_length);

  // GA

  T0V(GB_ENABLE
      , 0
      );

  //////////////////////////////////////////////////////////////////////////////
  // RS
  //////////////////////////////////////////////////////////////////////////////

  ib_rs_instructions(1);

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1)
      );
  T0V(VAP_OUT_VTX_FMT_1
      , VAP_OUT_VTX_FMT_1__TEX_0_COMP_CNT(4)
      );

  //

  T0V(ZB_CNTL, 0);
  T0V(ZB_ZSTENCILCNTL, 0);

  //

  //////////////////////////////////////////////////////////////////////////////
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_INVALTAGS, 0x00000000);

  T0V(TX_ENABLE
      , TX_ENABLE__TEX_0_ENABLE__ENABLE
      );

  int clamp = 2; // clamp to [0.0, 1.0]
  int txformat = 29; // TX_FMT_32F_32F_32F_32F
  ib_texture2(0,
              PARTICLE_POSITION_RELOC_INDEX,
              texture_width, texture_height,
              macrotile, microtile,
              clamp,
              txformat);

  // shaders

  ib_vap_stream_cntl__2();

  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(2)
      );

  ib_ga_us(&shaders.fragment[VERTEX_BUFFER_COPY_SHADER]);
  ib_vap_pvs(&shaders.vertex[VERTEX_BUFFER_COPY_SHADER]);

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__CLIP_DISABLE(1)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VPORT_X_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_SCALE_ENA(1)
      | VAP_VTE_CNTL__VTX_XY_FMT(1) // disable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(1)  // disable W division
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const int dwords_per_vtx = 2;

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(dwords_per_vtx)
      );

  const float vertices[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
  };
  const int vertex_count = 4;
  T3(_3D_DRAW_IMMD_2, (1 + vertex_count * dwords_per_vtx) - 1);
  TU( VAP_VF_CNTL__PRIM_TYPE(13) // quad list
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );
  for (int i = 0; i < vertex_count * 2; i++) {
    TF(vertices[i]);
  }

  //

  T0V(RB3D_DSTCACHE_CTLSTAT
      , RB3D_DSTCACHE_CTLSTAT__DC_FLUSH(0x2) // Flush dirty 3D data
      | RB3D_DSTCACHE_CTLSTAT__DC_FREE(0x2)  // Free 3D tags
      );

  T0V(ZB_ZCACHE_CTLSTAT
      , ZB_ZCACHE_CTLSTAT__ZC_FLUSH(1)
      | ZB_ZCACHE_CTLSTAT__ZC_FREE(1)
      );

  T0V(WAIT_UNTIL, 0x00020000);
}

int indirect_buffer(const shaders& shaders,
                    const floatbuffer_state& state,
                    //const particle * particles,
                    //const int particles_length,
                    float theta,
                    float * vertexbuffer_ptr,
                    int floatbuffer_width,
                    int floatbuffer_height)
{
  int width = 1600;
  int height = 1200;
  int pitch = width;

  ib_ix = 0;

  ib_generic_initialization();

  _copy_to_vertexbuffer(shaders,
                        state,
                        floatbuffer_width,
                        floatbuffer_height);

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

  //load_pvs_shaders(shaders.vertex, shaders.vertex_length);
  //load_us_shaders(shaders.fragment, shaders.fragment_length);

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
               state,
               //particles,
               //particles_length,
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

    particles[i].age = max_age * sinf(fi * rl * 2) * 0.5f + 0.5f;
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
                 + particles_length * vertex_count * 4 * (sizeof (float));

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

void reset_particle2(particle_position& position,
                     particle_velocity& velocity)
{
  //vec3 pos = normalize(p.position);

  position.position = normalize(vec3(velocity.velocity.x,
                                     0,
                                     velocity.velocity.z)) * 20.0f;

  //printf("position %f %f %f\n", p.position.x, p.position.y, p.position.z);

  velocity.velocity = vec3(velocity.velocity.x,
                           2.0f * velocity.delta,
                           velocity.velocity.z);

  //printf("velocity %f %f %f\n\n", p.velocity.x, p.velocity.y, p.velocity.z);
}

void init_particles2(void * position_ptr,
                     void * velocity_ptr,
                     const int particles_length)
{
  uint32_t state = 0x12345678;

  particle_position * position = (particle_position *)position_ptr;
  particle_velocity * velocity = (particle_velocity *)velocity_ptr;

  const float rl = 1.0f / (float)(particles_length);

  for (int i = 0; i < particles_length; i++) {
    float fi = ((float)i);

    float sx = xorshift32f(state) * 2.0f - 1.0f;
    float sy = xorshift32f(state) * 2.0f - 1.0f;
    float sz = xorshift32f(state) * 2.0f - 1.0f;

    float new_delta = xorshift32f(state) * 0.5f + 0.5f;

    float vx = xorshift32f(state) * 2.0f - 1.0f;
    float vz = xorshift32f(state) * 2.0f - 1.0f;

    float new_age = max_age * sinf(fi * rl * 2) * 0.5f + 0.5f;

    vec3 new_position = vec3(sx, sy, sz);
    vec3 new_velocity = normalize(vec3(vx * 0.5f, 0.0f, vz * 0.5f));

    position[i].position = new_position;
    position[i].age = new_age;
    velocity[i].velocity = new_velocity;
    velocity[i].delta = new_delta;

    reset_particle2(position[i], velocity[i]);
  }
}

floatbuffer_state create_floatbuffers(int fd,
                                      int length)
{
  floatbuffer_state state;
  int size = length * 4 * 4;
  for (int i = 0; i < 4; i++) {
    state.handles[i] = create_buffer(fd, size, &state.ptrs[i]);
  }

  init_particles2(state.ptrs[0],
                  state.ptrs[1],
                  length);

  state.flip = 0;
  state.length = length;

  return state;
}

int _floatbuffer(const shaders& shaders,
                 int input_reloc_index0,
                 int input_reloc_index1,
                 int output_reloc_index0,
                 int output_reloc_index1,
                 int floatbuffer_width,
                 int floatbuffer_height)
{
  int viewport_width = floatbuffer_width;
  int viewport_height = floatbuffer_height;
  int texture_width = floatbuffer_width;
  int texture_height = floatbuffer_height;
  /*
  float vx = ((float)viewport_width) * 0.5f;
  float vy = ((float)viewport_height) * 0.5f;
  float tx = 0.5f / ((float)texture_width);
  float ty = 0.5f / ((float)texture_height);
  */
  /*
  printf("tx ty: %f %f\n", tx, ty);

  printf("relocs: %d %d %d %d\n",
         input_reloc_index0,
         input_reloc_index1,
         output_reloc_index0,
         output_reloc_index1);
  */

  int macrotile = 0;
  int microtile = 0;

  ib_ix = 0;

  ib_generic_initialization();

  T0V(SC_SCISSOR0
      , SC_SCISSOR0__XS0(0)
      | SC_SCISSOR0__YS0(0)
      );
  T0V(SC_SCISSOR1
      , SC_SCISSOR1__XS1(viewport_width - 1)
      | SC_SCISSOR1__YS1(viewport_height - 1)
      );
  T0Vf(VAP_VPORT_XSCALE, (float)viewport_width);
  T0Vf(VAP_VPORT_YSCALE, (float)viewport_height);

  int colorformat = 7; // ARGB32323232

  ib_colorbuffer2(0,
                  output_reloc_index0,
                  viewport_width,
                  macrotile,
                  microtile,
                  colorformat);

  ib_colorbuffer2(1,
                  output_reloc_index1,
                  viewport_width,
                  macrotile,
                  microtile,
                  colorformat);

  T0V(US_OUT_FMT_0
      , US_OUT_FMT__OUT_FMT(21)  // C4_32_FP
      | US_OUT_FMT__C0_SEL__RED
      | US_OUT_FMT__C1_SEL__GREEN
      | US_OUT_FMT__C2_SEL__BLUE
      | US_OUT_FMT__C3_SEL__ALPHA
      | US_OUT_FMT__OUT_SIGN(0)
      );
  T0V(US_OUT_FMT_1
      , US_OUT_FMT__OUT_FMT(21)  // C4_32_FP
      | US_OUT_FMT__C0_SEL__RED
      | US_OUT_FMT__C1_SEL__GREEN
      | US_OUT_FMT__C2_SEL__BLUE
      | US_OUT_FMT__C3_SEL__ALPHA
      | US_OUT_FMT__OUT_SIGN(0)
      );
  T0V(US_OUT_FMT_2
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_3
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );

  // shaders
  load_pvs_shaders(shaders.vertex, shaders.vertex_length);
  load_us_shaders(shaders.fragment, shaders.fragment_length);

  // GA

  T0V(GB_ENABLE
      , 0
      );

  //////////////////////////////////////////////////////////////////////////////
  // RS
  //////////////////////////////////////////////////////////////////////////////

  ib_rs_instructions(1);

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1)
      );
  T0V(VAP_OUT_VTX_FMT_1
      , VAP_OUT_VTX_FMT_1__TEX_0_COMP_CNT(4)
      );

  //

  T0V(ZB_CNTL, 0);
  T0V(ZB_ZSTENCILCNTL, 0);

  //

  //////////////////////////////////////////////////////////////////////////////
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_INVALTAGS, 0x00000000);

  T0V(TX_ENABLE
      , TX_ENABLE__TEX_0_ENABLE__ENABLE
      | TX_ENABLE__TEX_1_ENABLE__ENABLE
      );

  int clamp = 2; // clamp to [0.0, 1.0]
  int txformat = 29; // TX_FMT_32F_32F_32F_32F
  ib_texture2(0,
              input_reloc_index0,
              texture_width, texture_height,
              macrotile, microtile,
              clamp,
              txformat);

  ib_texture2(1,
              input_reloc_index1,
              texture_width, texture_height,
              macrotile, microtile,
              clamp,
              txformat);

  // shaders

  ib_vap_stream_cntl__2();

  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(6)
      );

  ib_ga_us(&shaders.fragment[PARTICLE_PHYSICS_SHADER]);
  ib_vap_pvs(&shaders.vertex[PARTICLE_PHYSICS_SHADER]);

  const float vertex_consts[] = {
    //-tx, -ty, 0, 0,
    0, 0, 0, 0,
  };
  const int vertex_consts_size = (sizeof (vertex_consts));
  ib_vap_pvs_const_cntl(vertex_consts, vertex_consts_size);

  // fragment constants
  //const vec3 velocity_scale = vec3(0.003f, 0.01f, 0.003f);
  const vec3 velocity_scale = vec3(0.9f, 5.0f, 0.9f);
  const float delta_age = -0.01f;
  const float velocity_attenuation = -0.6f; // multiplied by velocity.y after bounce
  const float gravity = -0.04f;
  const float max_age = 3.0f;
  const float reset_radius = 20.0f;
  const float fragment_consts[] = {
    // 0:
    velocity_scale.x, velocity_scale.y, velocity_scale.z, delta_age,
    // 1:
    velocity_attenuation, gravity, max_age, reset_radius,
  };
  int fragment_consts_length = (sizeof (fragment_consts)) / (sizeof (fragment_consts[0]));
  ib_ga_consts(fragment_consts, fragment_consts_length, 0);

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__CLIP_DISABLE(1)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VPORT_X_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_SCALE_ENA(1)
      | VAP_VTE_CNTL__VTX_XY_FMT(1) // disable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(1)  // disable W division
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const int dwords_per_vtx = 2;

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(dwords_per_vtx)
      );

  const float vertices[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
  };
  const int vertex_count = 4;
  T3(_3D_DRAW_IMMD_2, (1 + vertex_count * dwords_per_vtx) - 1);
  TU( VAP_VF_CNTL__PRIM_TYPE(13) // quad list
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );
  for (int i = 0; i < vertex_count * 2; i++) {
    TF(vertices[i]);
  }

  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ib_ix % 8) != 0) {
    TU(0x80000000);
  }

  return ib_ix;
}

void check_particles2(floatbuffer_state& state)
{
  assert(state.flip == 0 || state.flip == 1);
  int fb_input = state.flip * 2;
  int fb_output = (!state.flip) * 2;

  particle_position * in_pos = (particle_position *)state.ptrs[fb_input + 0];
  particle_velocity * in_vel = (particle_velocity *)state.ptrs[fb_input + 1];

  particle_position * out_pos = (particle_position *)state.ptrs[fb_output + 0];
  particle_velocity * out_vel = (particle_velocity *)state.ptrs[fb_output + 1];

  int unequal = 0;

  for (int i = 0; i < state.length; i++) {
    bool pos_eq =
      (in_pos[i].position.x == out_pos[i].position.x) &&
      (in_pos[i].position.y == out_pos[i].position.y) &&
      (in_pos[i].position.z == out_pos[i].position.z);

    bool vel_eq =
      (in_vel[i].velocity.x == out_vel[i].velocity.x) &&
      (in_vel[i].velocity.y == out_vel[i].velocity.y) &&
      (in_vel[i].velocity.z == out_vel[i].velocity.z);

    if (!(pos_eq && vel_eq)) {
      unequal += 1;
      printf("[%d] %d %d %d %d\n", i, fb_input + 0, fb_input + 1, fb_output + 0, fb_output + 1);
      printf("   in pos (% 3.04f % 3.04f % 3.04f)\n", in_pos[i].position.x, in_pos[i].position.y, in_pos[i].position.z);
      printf("   in vel (% 3.04f % 3.04f % 3.04f)\n", in_vel[i].velocity.x, in_vel[i].velocity.y, in_vel[i].velocity.z);
      printf("  out pos (% 3.04f % 3.04f % 3.04f)\n", out_pos[i].position.x, out_pos[i].position.y, out_pos[i].position.z);
      printf("  out vel (% 3.04f % 3.04f % 3.04f)\n", out_vel[i].velocity.x, out_vel[i].velocity.y, out_vel[i].velocity.z);
    }
  }
  printf("unequal %d\n", unequal);
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

  const int floatbuffer_width = 64;
  const int floatbuffer_height = 64;
  floatbuffer_state state = create_floatbuffers(fd, floatbuffer_width * floatbuffer_height);

  vertexbuffer_handle = init_particles_vertexbuffer(fd, state.length, &vertexbuffer_ptr);
  fprintf(stderr, "vertexbuffer handle %d\n", vertexbuffer_handle);

  while (true) {
    assert(state.flip == 0 || state.flip == 1);
    int fb_input = state.flip * 2;
    int fb_output = (!state.flip) * 2;

    {
      int ib_dwords = _floatbuffer(shaders,
                                   fb_input  + 0, // input_reloc_index0,
                                   fb_input  + 1, // input_reloc_index1,
                                   fb_output + 0, // output_reloc_index0,
                                   fb_output + 1, // output_reloc_index1,
                                   floatbuffer_width,
                                   floatbuffer_height);

      int ret = drm_radeon_cs2(fd,
                               state.handles,
                               4,
                               ib_dwords);
      assert(ret != -1);
      //printf("floatbuffer return %d\n", ret);
      //check_particles2(state);
    }

    {
      int ib_dwords = indirect_buffer(shaders,
                                      state,
                                      theta,
                                      vertexbuffer_ptr,
                                      floatbuffer_width,
                                      floatbuffer_height);

      assert(textures_length == 2);
      int particle_position_handle = state.handles[fb_output + 0];
      int handles[] = {
        colorbuffer_handle[colorbuffer_ix], // 0
        zbuffer_handle,                     // 1
        vertexbuffer_handle,                // 2
        texturebuffer_handle[0],            // 3
        texturebuffer_handle[1],            // 4
        particle_position_handle,           // 5
      };
      int handles_length = (sizeof (handles)) / (sizeof (handles[0]));

      int ret = drm_radeon_cs2(fd,
                               handles,
                               handles_length,
                               ib_dwords);

      if (ret == -1)
        break;
    }

    primary_surface_address(rmmio, colorbuffer_ix);

    // next state
    theta += 0.01f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;

    state.flip = (state.flip + 1) & 1;
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
