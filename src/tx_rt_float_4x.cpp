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

#define TEXTURE_TILE_SHADER 0

const char * vertex_shader_paths[] = {
  "tx_rt_float.vs.bin",
};
const int vertex_shader_paths_length = (sizeof (vertex_shader_paths)) / (sizeof (vertex_shader_paths[0]));
const char * fragment_shader_paths[] = {
  "tx_rt_float.fs.bin",
};
const int fragment_shader_paths_length = (sizeof (fragment_shader_paths)) / (sizeof (fragment_shader_paths[0]));

#define PARTICLE_TEXTURE 0

const char * textures[] = {
  "../texture/butterfly_1024x1024_rgba8888.data",
};
const int textures_length = (sizeof (textures)) / (sizeof (textures[0]));

struct shaders {
  struct shader_offset * vertex;
  struct shader_offset * fragment;
  int vertex_length;
  int fragment_length;
};

const int floatbuffer_width = 64;
const int floatbuffer_height = 64;

int _floatbuffer(const shaders& shaders,
                 int input_reloc_index,
                 int output_reloc_index)
{
  int viewport_width = floatbuffer_width * 4;
  int viewport_height = floatbuffer_height;
  int texture_width = floatbuffer_width;
  int texture_height = floatbuffer_height;
  float vx = ((float)viewport_width) * 0.5f;
  float vy = ((float)viewport_height) * 0.5f;
  float tx = 0.5f / ((float)texture_width);
  float ty = 0.5f / ((float)texture_height);

  ib_ix = 0;

  ib_generic_initialization();

  printf("vp %d %d \n", viewport_width, viewport_height);

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

  ib_colorbuffer(output_reloc_index, viewport_width, 0, 0); // macrotile, microtile

  T0V(RB3D_COLORPITCH0
      , RB3D_COLORPITCH__COLORPITCH(viewport_width >> 1)
      | RB3D_COLORPITCH__COLORTILE(0)
      | RB3D_COLORPITCH__COLORMICROTILE(0)
      | RB3D_COLORPITCH__COLORFORMAT__ARGB32323232
      );

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

  int macrotile = 0;
  int microtile = 0;
  int clamp = 2; // clamp to [0.0, 1.0]
  ib_texture__1_float32(input_reloc_index,
                        texture_width, texture_height,
                        macrotile, microtile,
                        clamp);

  ib_vap_stream_cntl__2();

  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(1)
      );

  ib_ga_us(&shaders.fragment[TEXTURE_TILE_SHADER]);
  ib_vap_pvs(&shaders.vertex[TEXTURE_TILE_SHADER]);

  const float vertex_consts[] = {
    tx, ty, 0, 0,
  };
  const int vertex_consts_size = (sizeof (vertex_consts));
  ib_vap_pvs_const_cntl(vertex_consts, vertex_consts_size);

  // fragment constants

  const float fragment_consts[] = {
    1234.0f, 0, 0, 0,
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
  TU( VAP_VF_CNTL__PRIM_TYPE(5) // triangle fan
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

  return ib_ix;
}

void floatbuffer_data(void * ptr, int size)
{
  float * f32 = (float*)ptr;
  int vector_length = size / (4 * 4);

  int offset = 16384;
  for (int i = 0; i < vector_length; i++) {
    f32[i * 4 + 0] = offset--;
    f32[i * 4 + 1] = offset--;
    f32[i * 4 + 2] = offset--;
    f32[i * 4 + 3] = offset--;
  }
}

void floatbuffer_compare(void * a, void * b, int size)
{
  float * a_f32 = (float*)a;
  float * b_f32 = (float*)b;
  int vector_length = size / (4 * 4);

  int matches = 0;

  int ix = 0;
  for (int i = 0; i < vector_length; i++) {
    for (int j = 0; j < 4; j++) {
      if (   (a_f32[i * 4 + 0] != b_f32[ix * 4 + 0])
             || (a_f32[i * 4 + 1] != b_f32[ix * 4 + 1])
             || (a_f32[i * 4 + 2] != b_f32[ix * 4 + 2])
             || (a_f32[i * 4 + 3] != b_f32[ix * 4 + 3])) {
        printf("a[%d] = [% 2.02f  % 2.02f  % 2.02f  % 2.02f] ; ", i,
               a_f32[i * 4 + 0], a_f32[i * 4 + 1], a_f32[i * 4 + 2], a_f32[i * 4 + 3]);
        printf("b[%d] = [% 2.02f  % 2.02f  % 2.02f  % 2.02f] \n", i,
               b_f32[ix * 4 + 0], b_f32[ix * 4 + 1], b_f32[ix * 4 + 2], b_f32[ix * 4 + 3]);
      } else {
        matches += 1;
      }
      ix++;
    }
  }
  printf("vector_length %d matches %d\n", vector_length, matches);
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
  const int floatbuffer_size = floatbuffer_width * floatbuffer_height * 4 * 4;
  const int floatbuffer_count = 2;
  int colorbuffer_handle[2];
  int zbuffer_handle;
  int * texturebuffer_handle;
  int flush_handle;
  int floatbuffer_handle[floatbuffer_count];

  void * colorbuffer_ptr[2];
  void * floatbuffer_ptr[floatbuffer_count];
  void * zbuffer_ptr;

  // colorbuffer
  colorbuffer_handle[0] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[0]);
  colorbuffer_handle[1] = create_buffer(fd, colorbuffer_size, &colorbuffer_ptr[1]);
  zbuffer_handle = create_buffer(fd, colorbuffer_size, &zbuffer_ptr);
  flush_handle = create_flush_buffer(fd);
  texturebuffer_handle = load_textures(fd, textures, textures_length);

  floatbuffer_handle[0] = create_buffer(fd, floatbuffer_size, &floatbuffer_ptr[0]);
  floatbuffer_handle[1] = create_buffer(fd, floatbuffer_size * 4, &floatbuffer_ptr[1]);

  fprintf(stderr, "colorbuffer handle[0] %d\n", colorbuffer_handle[0]);
  fprintf(stderr, "colorbuffer handle[1] %d\n", colorbuffer_handle[1]);
  fprintf(stderr, "floatbuffer handle[0] %d\n", floatbuffer_handle[0]);
  fprintf(stderr, "floatbuffer handle[1] %d\n", floatbuffer_handle[1]);
  fprintf(stderr, "zbuffer handle %d\n", zbuffer_handle);

  int colorbuffer_ix = 0;

  floatbuffer_data(floatbuffer_ptr[0], floatbuffer_size);

  while (true) {
    int ib_dwords = _floatbuffer(shaders,
                                 TEXTUREBUFFER_RELOC_INDEX + 0, // input
                                 COLORBUFFER_RELOC_INDEX);      // output

    int ret = drm_radeon_cs(fd,
                            floatbuffer_handle[1],
                            zbuffer_handle,
                            flush_handle,
                            floatbuffer_handle,
                            floatbuffer_count,
                            ib_dwords);
    if (ret == -1)
      break;

    break;
  }

  floatbuffer_compare(floatbuffer_ptr[0], floatbuffer_ptr[1], floatbuffer_size);

  close(fd);
}
