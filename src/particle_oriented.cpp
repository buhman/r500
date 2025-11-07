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
#include "../model/plane.h"

#define CLEAR_SHADER 0
#define PLANE_SHADER 1
#define PARTICLE_SHADER 2
#define TEXTURE_TILE_SHADER 3

const char * vertex_shader_paths[] = {
  "clear.vs.bin",
  "particle_plane.vs.bin",
  "particle_particle.vs.bin",
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

void _3d_clear(struct shaders& shaders)
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

void _3d_plane_inner(mat4x4 trans)
{
  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const model * model = &plane_model;
  const object * obj = model->object[0];
  const int triangle_count = obj->triangle_count;
  const int vertex_count = triangle_count * 3;

  const int dwords_per_vtx = 5;

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(dwords_per_vtx)
      );

  T3(_3D_DRAW_IMMD_2, (1 + vertex_count * dwords_per_vtx) - 1);
  TU( VAP_VF_CNTL__PRIM_TYPE(4)
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );

  for (int i = 0; i < triangle_count; i++) {
    for (int j = 0; j < 3; j++) {
      vec3 p = model->position[obj->triangle[i][j].position];
      vec2 t = model->texture[obj->triangle[i][j].texture];

      TF(p.x);
      TF(p.y);
      TF(p.z);
      TF(t.x);
      TF(t.y);
    }
  }
}

void _3d_plane(struct shaders& shaders,
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

  ib_vap_stream_cntl__32();

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

  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);

  //////////////////////////////////////////////////////////////////////////////
  // consts
  //////////////////////////////////////////////////////////////////////////////

  const float consts[] = {
    // 0
    trans[0][0], trans[0][1], trans[0][2], trans[0][3],
    trans[1][0], trans[1][1], trans[1][2], trans[1][3],
    trans[2][0], trans[2][1], trans[2][2], trans[2][3],
    trans[3][0], trans[3][1], trans[3][2], trans[3][3],
  };
  ib_vap_pvs_const_cntl(consts, (sizeof (consts)));

  // plane_inner

  _3d_plane_inner(trans);
}

void _3d_particle(struct shaders& shaders,
                  const mat4x4& world_to_clip,
                  const mat4x4& world_to_view,
                  float theta)
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

  ib_zbuffer(ZBUFFER_RELOC_INDEX, 1600, 1); // less

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

  const float consts[] = {
    // 0
    trans[0][0], trans[0][1], trans[0][2], trans[0][3],
    trans[1][0], trans[1][1], trans[1][2], trans[1][3],
    trans[2][0], trans[2][1], trans[2][2], trans[2][3],
    trans[3][0], trans[3][1], trans[3][2], trans[3][3],

    // 4: particle_position
    0, 0, 0, 0,

    // 5: dx (right)
    local_to_view[0][0], local_to_view[0][1], local_to_view[0][2], 0,

    // 6: dy (up)
    local_to_view[1][0], local_to_view[1][1], local_to_view[1][2], 0,
  };
  ib_vap_pvs_const_cntl(consts, (sizeof (consts)));

  // plane_inner

  _3d_plane_inner(trans);
}

int indirect_buffer(shaders& shaders,
                    float theta)
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
  _3d_particle(shaders, world_to_clip, world_to_view, theta);
  //_3d_zbuffer(shaders);

  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ib_ix % 8) != 0) {
    TU(0x80000000);
  }

  return ib_ix;
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
  int flush_handle;

  void * colorbuffer_ptr[2];
  void * zbuffer_ptr;

  // colorbuffer
  colorbuffer_handle[0] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[0]);
  colorbuffer_handle[1] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[1]);
  zbuffer_handle = create_buffer(fd, colorbuffer_size, &zbuffer_ptr);
  flush_handle = create_flush_buffer(fd);
  texturebuffer_handle = load_textures(fd, textures, textures_length);

  fprintf(stderr, "colorbuffer handle[0] %d\n", colorbuffer_handle[0]);
  fprintf(stderr, "colorbuffer handle[1] %d\n", colorbuffer_handle[1]);
  fprintf(stderr, "zbuffer handle %d\n", zbuffer_handle);

  int colorbuffer_ix = 0;
  float theta = PI * 0.5;

  while (true) {
    int ib_dwords = indirect_buffer(shaders, theta);

    drm_radeon_cs(fd,
                  colorbuffer_handle[colorbuffer_ix],
                  zbuffer_handle,
                  flush_handle,
                  texturebuffer_handle,
                  textures_length,
                  ib_dwords);

    primary_surface_address(rmmio, colorbuffer_ix);

    // next state
    theta += 0.01f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;
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
