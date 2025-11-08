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
  "tx_rt.vs.bin",
};
const int vertex_shader_paths_length = (sizeof (vertex_shader_paths)) / (sizeof (vertex_shader_paths[0]));
const char * fragment_shader_paths[] = {
  "tx_rt.fs.bin",
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

int _tile_texture(const shaders& shaders,
                  int input_reloc_index,
                  int output_reloc_index)
{
  int viewport_width = 1600;
  int viewport_height = 1200;
  int texture_width = 1024;
  int texture_height = 1024;
  float vx = ((float)viewport_width) * 0.5f;
  float vy = ((float)viewport_height) * 0.5f;
  float tx = 0.5f / ((float)texture_width);
  float ty = 0.5f / ((float)texture_height);

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

  ib_colorbuffer(output_reloc_index, viewport_width, 0, 0); // macrotile, microtile

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
  ib_texture__1(input_reloc_index,
                texture_width, texture_height,
                macrotile, microtile,
                clamp);

  ib_vap_stream_cntl__2();

  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(1)
      );

  ib_ga_us(&shaders.fragment[TEXTURE_TILE_SHADER]);
  ib_vap_pvs(&shaders.vertex[TEXTURE_TILE_SHADER]);

  // fragment constants
  const float fragment_consts[] = {
    tx, ty, 0, 0,
  };
  int fragment_consts_length = (sizeof (fragment_consts)) / (sizeof (fragment_consts[0]));

  T0V(GA_US_VECTOR_INDEX
      , GA_US_VECTOR_INDEX__INDEX(0)
      | GA_US_VECTOR_INDEX__TYPE(1)
      );
  T0_ONE_REG(GA_US_VECTOR_DATA, (fragment_consts_length - 1));
  for (int i = 0; i < fragment_consts_length; i++)
    TF(fragment_consts[i]);

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
  // GA POINT SIZE
  //////////////////////////////////////////////////////////////////////////////

  T0V(GA_POINT_SIZE
      , GA_POINT_SIZE__HEIGHT((int)(vy * 12.0f))
      | GA_POINT_SIZE__WIDTH((int)(vx * 12.0f))
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
    //int ib_dwords = indirect_buffer(shaders, theta);
    int ib_dwords = _tile_texture(shaders,
                                  TEXTUREBUFFER_RELOC_INDEX, // input
                                  COLORBUFFER_RELOC_INDEX);  // output

    int ret = drm_radeon_cs(fd,
                            colorbuffer_handle[colorbuffer_ix],
                            zbuffer_handle,
                            flush_handle,
                            texturebuffer_handle,
                            textures_length,
                            ib_dwords);
    if (ret == -1)
      break;

    primary_surface_address(rmmio, colorbuffer_ix);

    // next state
    theta += 0.01f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;

    break;
  }

  close(fd);
}
