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
#include "../model/cubesphere.h"
#include "../model/cubesphere_indexed.h"

#define CLEAR_SHADER 0
#define CUBESPHERE_SHADER 1
#define LIGHT_SHADER 2

#define INDEXBUFFER_RELOC_INDEX 4

const char * vertex_shader_paths[] = {
  "clear.vs.bin",
  "matrix_cubesphere.vs.bin",
  "light.vs.bin",
};
const int vertex_shader_paths_length = (sizeof (vertex_shader_paths)) / (sizeof (vertex_shader_paths[0]));
const char * fragment_shader_paths[] = {
  "clear.fs.bin",
  "matrix_cubesphere.fs.bin",
  "light.fs.bin"
};
const int fragment_shader_paths_length = (sizeof (fragment_shader_paths)) / (sizeof (fragment_shader_paths[0]));

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

void _3d_cube_inner(mat4x4 trans,
                    mat4x4 world_trans,
                    vec4 light_pos)
{
  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  const float consts[] = {
    // 0
    trans[0][0], trans[0][1], trans[0][2], trans[0][3],
    trans[1][0], trans[1][1], trans[1][2], trans[1][3],
    trans[2][0], trans[2][1], trans[2][2], trans[2][3],
    trans[3][0], trans[3][1], trans[3][2], trans[3][3],

    // 4
    world_trans[0][0], world_trans[0][1], world_trans[0][2], world_trans[0][3],
    world_trans[1][0], world_trans[1][1], world_trans[1][2], world_trans[1][3],
    world_trans[2][0], world_trans[2][1], world_trans[2][2], world_trans[2][3],
    world_trans[3][0], world_trans[3][1], world_trans[3][2], world_trans[3][3],

    // 8
    light_pos.x, light_pos.y, light_pos.z, light_pos.w,
  };
  ib_vap_pvs_const_cntl(consts, (sizeof (consts)));

  //////////////////////////////////////////////////////////////////////////////
  // AOS
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);

  T3(_3D_LOAD_VBPNTR, (6 - 1));
  TU( // VAP_VTX_NUM_ARRAYS
      VAP_VTX_NUM_ARRAYS__VTX_NUM_ARRAYS(3)
    | VAP_VTX_NUM_ARRAYS__VC_FORCE_PREFETCH(1)
    );
  TU( // VAP_VTX_AOS_ATTR01
      VAP_VTX_AOS_ATTR__VTX_AOS_COUNT0(3)
    | VAP_VTX_AOS_ATTR__VTX_AOS_STRIDE0(8)
    | VAP_VTX_AOS_ATTR__VTX_AOS_COUNT1(2)
    | VAP_VTX_AOS_ATTR__VTX_AOS_STRIDE1(8)
    );
  TU( // VAP_VTX_AOS_ADDR0
    (4 * 0)
    );
  TU( // VAP_VTX_AOS_ADDR1
    (4 * 3)
    );
  TU( // VAP_VTX_AOS_ATTR23
      VAP_VTX_AOS_ATTR__VTX_AOS_COUNT0(3)
    | VAP_VTX_AOS_ATTR__VTX_AOS_STRIDE0(8)
    );
  TU( // VAP_VTX_AOS_ADDR2
    (4 * 5)
    );

  // VAP_VTX_AOS_ADDR is an absolute address in VRAM. However, DRM_RADEON_CS
  // modifies this to be an offset relative to the GEM buffer handles given via
  // NOP:
  T3(_NOP, 0);
  TU(VERTEXBUFFER_RELOC_INDEX * 4); // index into relocs array for VAP_VTX_AOS_ADDR0
  T3(_NOP, 0);
  TU(VERTEXBUFFER_RELOC_INDEX * 4); // index into relocs array for VAP_VTX_AOS_ADDR1
  T3(_NOP, 0);
  TU(VERTEXBUFFER_RELOC_INDEX * 4); // index into relocs array for VAP_VTX_AOS_ADDR2

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const int vertex_count = cubesphere_Cube_triangles_length;

  T3(_3D_DRAW_INDX_2, (1 - 1));
  TU( VAP_VF_CNTL__PRIM_TYPE(4)
    | VAP_VF_CNTL__PRIM_WALK(1) // indexes in command stream , vertices from memory
    | VAP_VF_CNTL__INDEX_SIZE(1) // 32 bits per index
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(vertex_count)
    );

  T3(_INDX_BUFFER, (3 - 1));
  TU( // INDX_BUFFER__0
      INDX_BUFFER__0__ONE_REG_WR(1)
    | INDX_BUFFER__0__SKIP_COUNT(0)
    | INDX_BUFFER__0__DESTINATION(VAP_PORT_IDX0 >> 2)
    );
  TU( // INDX_BUFFER__1
      INDX_BUFFER__1__BUFFER_BASE(0) // replaced by reloc
    );
  TU( // INDX_BUFFER__2
      INDX_BUFFER__2__BUFFER_SIZE(vertex_count) // in dwords
    );
  T3(_NOP, 0);
  TU(INDEXBUFFER_RELOC_INDEX * 4); // index into relocs array for INDX_BUFFER__1__BUFFER_BASE
}

void _3d_light_inner(mat4x4 trans)
{
  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  const vec4 color = {1, 1, 0, 1};

  const float consts[] = {
    // 0
    trans[0][0], trans[0][1], trans[0][2], trans[0][3],
    trans[1][0], trans[1][1], trans[1][2], trans[1][3],
    trans[2][0], trans[2][1], trans[2][2], trans[2][3],
    trans[3][0], trans[3][1], trans[3][2], trans[3][3],

    // 4
    color[0], color[1], color[2], color[2],
  };
  ib_vap_pvs_const_cntl(consts, (sizeof (consts)));

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const model * model = &cubesphere_model;
  const object * obj = model->object[0];
  const int triangle_count = obj->triangle_count;
  const int vertex_count = triangle_count * 3;

  int dwords_per_vtx = 3;

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

      TF(p.x);
      TF(p.y);
      TF(p.z);
    }
  }
}

vec3 _3d_light(struct shaders& shaders,
               const mat4x4& view_to_clip,
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

  ib_texture__0();

  ib_vap_stream_cntl__3();

  // shaders
  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(1)
      );
  ib_ga_us(&shaders.fragment[LIGHT_SHADER]);
  ib_vap_pvs(&shaders.vertex[LIGHT_SHADER]);

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
      | VAP_VTE_CNTL__VPORT_Z_SCALE_ENA(0)
      | VAP_VTE_CNTL__VPORT_Z_OFFSET_ENA(0)
      | VAP_VTE_CNTL__VTX_XY_FMT(0) // enable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(1)  // disable W division
      | VAP_VTE_CNTL__VTX_W0_FMT(1)
      | VAP_VTE_CNTL__SERIAL_PROC_ENA(0)
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

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

  // light
  mat4x4 t1 = translate(vec3(1, 0, 0));
  mat4x4 s = scale(0.1f);
  mat4x4 rz = rotate_y(theta * 2.f);

  mat4x4 world_trans = rz * t1 * s;

  mat4x4 trans = view_to_clip * world_trans;

  _3d_light_inner(trans);

  vec3 light_pos = world_trans * light_pos;

  return light_pos;
}

void _3d_cube(struct shaders& shaders,
              const mat4x4& view_to_clip,
              float theta,
              const vec3& light_pos)
{
  ib_rs_instructions(4);

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , VAP_OUT_VTX_FMT_1__TEX_0_COMP_CNT(4)
      | VAP_OUT_VTX_FMT_1__TEX_1_COMP_CNT(4)
      | VAP_OUT_VTX_FMT_1__TEX_2_COMP_CNT(4)
      | VAP_OUT_VTX_FMT_1__TEX_3_COMP_CNT(4));

  //

  ib_zbuffer(ZBUFFER_RELOC_INDEX, 1600, 1); // less

  int width = 1024;
  int height = 1024;
  int macrotile = 0;
  int microtile = 0;
  int clamp = 0; // wrap/repeat
  ib_texture__1(TEXTUREBUFFER_RELOC_INDEX,
                width, height,
                macrotile, microtile,
                clamp);

  ib_vap_stream_cntl__323();

  // shaders
  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(4)
      );
  ib_ga_us(&shaders.fragment[CUBESPHERE_SHADER]);
  ib_vap_pvs(&shaders.vertex[CUBESPHERE_SHADER]);

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
      | VAP_VTE_CNTL__VPORT_Z_SCALE_ENA(0)
      | VAP_VTE_CNTL__VPORT_Z_OFFSET_ENA(0)
      | VAP_VTE_CNTL__VTX_XY_FMT(0) // enable W division
      | VAP_VTE_CNTL__VTX_Z_FMT(1)  // disable W division
      | VAP_VTE_CNTL__VTX_W0_FMT(1)
      | VAP_VTE_CNTL__SERIAL_PROC_ENA(0)
      );

  T0V(VAP_CNTL_STATUS
      , VAP_CNTL_STATUS__PVS_BYPASS(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP INDEX
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_INDEX_OFFSET, 0);

  T0V(VAP_VF_MAX_VTX_INDX
      , VAP_VF_MAX_VTX_INDX__MAX_INDX(124)
      );
  T0V(VAP_VF_MIN_VTX_INDX
      , VAP_VF_MIN_VTX_INDX__MIN_INDX(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // matrix
  //////////////////////////////////////////////////////////////////////////////

  // cube
  mat4x4 rx = rotate_x(1 * theta * 0.5f);
  mat4x4 ry = rotate_y(0 * theta * 0.8f + 1.4f);
  mat4x4 s = scale(0.9f);

  mat4x4 world_trans = rx * ry * s;

  mat4x4 trans = view_to_clip * world_trans;

  _3d_cube_inner(trans, world_trans, light_pos);
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
  mat4x4 p = perspective(0.01f, 5.0f,
                         0.001f, 0.999f,
                         0.5f, 2.0f);
  mat4x4 t = translate(vec3(0, 0, 3));
  mat4x4 view_to_clip = aspect * p * t;

  _3d_clear(shaders);
  vec3 light_pos = _3d_light(shaders, view_to_clip, theta);
  _3d_cube(shaders, view_to_clip, theta, light_pos);

  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ib_ix % 8) != 0) {
    TU(0x80000000);
  }

  return ib_ix;
}

const char * textures[] = {
  "../texture/butterfly_1024x1024_rgba8888.data",
};
const int textures_length = (sizeof (textures)) / (sizeof (textures[0]));

void transfer_vertex_index(void * vertexbuffer_ptr,
                           void * indexbuffer_ptr)
{
  for (int i = 0; i < cubesphere_vertices_length; i++) {
    ((float *)vertexbuffer_ptr)[i] = cubesphere_vertices[i];
  }

  for (int i = 0; i < cubesphere_vertices_length; i++) {
    ((int *)indexbuffer_ptr)[i] = cubesphere_Cube_triangles[i];
  }
  asm volatile ("" ::: "memory");
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
  int vertexbuffer_handle;
  int * texturebuffer_handle;
  int indexbuffer_handle;

  void * vertexbuffer_ptr;
  void * indexbuffer_ptr;
  void * colorbuffer_ptr[2];
  void * zbuffer_ptr;

  // cubesphere_Cube_triangles_length, cubesphere_vertices_length
  const int vertexbuffer_size = cubesphere_vertices_length * 4;
  const int indexbuffer_size = cubesphere_Cube_triangles_length * 4;

  // colorbuffer
  colorbuffer_handle[0] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[0]);
  colorbuffer_handle[1] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[1]);
  zbuffer_handle = create_buffer(fd, colorbuffer_size, &zbuffer_ptr);
  vertexbuffer_handle = create_buffer(fd, vertexbuffer_size, &vertexbuffer_ptr);
  texturebuffer_handle = load_textures(fd, textures, textures_length);
  indexbuffer_handle = create_buffer(fd, indexbuffer_size, &indexbuffer_ptr);

  fprintf(stderr, "colorbuffer handle[0] %d\n", colorbuffer_handle[0]);
  fprintf(stderr, "colorbuffer handle[1] %d\n", colorbuffer_handle[1]);
  fprintf(stderr, "zbuffer handle %d\n", zbuffer_handle);

  transfer_vertex_index(vertexbuffer_ptr, indexbuffer_ptr);

  int colorbuffer_ix = 0;
  float theta = 0;

  while (true) {
    int ib_dwords = indirect_buffer(shaders, theta);

    int handles[] = {
      colorbuffer_handle[colorbuffer_ix], // 0
      zbuffer_handle,                     // 1
      vertexbuffer_handle,                // 2
      texturebuffer_handle[0],            // 3
      indexbuffer_handle,                 // 4
    };
    int handles_length = (sizeof (handles)) / (sizeof (handles[0]));

    int ret = drm_radeon_cs2(fd,
                             handles,
                             handles_length,
                             ib_dwords);
    if (ret == -1)
      break;

    primary_surface_address(rmmio, colorbuffer_ix);

    // next state
    theta += 0.01f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;
  }

  close(fd);
}
