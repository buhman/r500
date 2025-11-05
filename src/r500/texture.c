#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "3d_registers.h"
#include "3d_registers_bits.h"

#include "../drm/buffer.h"
#include "../drm/drm.h"
#include "../file.h"

#include "texture.h"
#include "shader.h"
#include "indirect_buffer.h"

#define TEXTURE_TILE_SHADER 0

static int _tile_texture(const struct shaders * shaders,
                         int input_reloc_index,
                         int output_reloc_index)
{
  int width = 1024;
  int height = 1024;
  int pitch = width;
  float x = (float)width * 0.5f;
  float y = (float)height * 0.5f;

  ib_ix = 0;

  ib_generic_initialization();

  ib_viewport(width, height);
  ib_colorbuffer(output_reloc_index, pitch, 1, 1); // macrotile, microtile

  T0V(US_OUT_FMT_0
      , US_OUT_FMT__OUT_FMT(0)  // C4_8
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

  // GA

  T0V(GB_ENABLE
    , GB_ENABLE__POINT_STUFF_ENABLE(1)
    | GB_ENABLE__TEX0_SOURCE(2) // stuff with source texture coordinates s,t
    );

  T0Vf(GA_POINT_S0, 0.0f);
  T0Vf(GA_POINT_T0, 1.0f);
  T0Vf(GA_POINT_S1, 1.0f);
  T0Vf(GA_POINT_T1, 0.0f);

  //////////////////////////////////////////////////////////////////////////////
  // RS
  //////////////////////////////////////////////////////////////////////////////

  int rs_instructions = 1;

  //ib_rs_instructions(0);

  T0V(RS_IP_0
    , RS_IP__TEX_PTR_S(0)
    | RS_IP__TEX_PTR_T(1)
    | RS_IP__TEX_PTR_R(62) // constant 0.0
    | RS_IP__TEX_PTR_Q(63) // constant 1.0
    );

  T0V(RS_COUNT
      , RS_COUNT__IT_COUNT(2)
      | RS_COUNT__IC_COUNT(0)
      | RS_COUNT__W_ADDR(0)
      | RS_COUNT__HIRES_EN(1)
      );

  T0V(RS_INST_COUNT
      , RS_INST_COUNT__INST_COUNT(rs_instructions - 1));

  T0V(RS_INST_0
      , RS_INST__TEX_ID(0)
      | RS_INST__TEX_CN(1)
      | RS_INST__TEX_ADDR(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP OUT
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1)
      );
  T0V(VAP_OUT_VTX_FMT_1
      , 0
      );

  //

  T0V(ZB_CNTL, 0);
  T0V(ZB_ZSTENCILCNTL, 0);

  //

  int macrotile = 0;
  int microtile = 0;
  int clamp = 2; // clamp to [0.0, 1.0]
  ib_texture__1(input_reloc_index,
                width, height,
                macrotile, microtile,
                clamp);

  ib_vap_stream_cntl__2();

  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(1)
      );

  ib_ga_us(&shaders->fragment[TEXTURE_TILE_SHADER]);
  ib_vap_pvs(&shaders->vertex[TEXTURE_TILE_SHADER]);

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
      , GA_POINT_SIZE__HEIGHT((int)(x * 12.0f))
      | GA_POINT_SIZE__WIDTH((int)(y * 12.0f))
      );

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  const int dwords_per_vtx = 2;

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(dwords_per_vtx)
      );

  const float center[] = {
    x, y,
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

  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ib_ix % 8) != 0) {
    TU(0x80000000);
  }

  return ib_ix;
}

int * load_textures(int fd,
                    const char ** textures,
                    int textures_length)
{
  int * texturebuffer_handle = (int *)malloc((sizeof (int)) * textures_length);

  for (int i = 0; i < textures_length; i++) {
    int size = 0;
    void * buf = file_read(textures[i], &size);
    assert(buf != NULL);

    printf("load texture[%d]: %d\n", i, size);

    void * ptr = NULL;
    int handle = create_buffer(fd, size, &ptr);

    for (int i = 0; i < size / 4; i++) {
      ((uint32_t*)ptr)[i] = ((uint32_t*)buf)[i];
    }
    asm volatile ("" ::: "memory");
    free(buf);
    munmap(ptr, size);

    texturebuffer_handle[i] = handle;
  }

  return texturebuffer_handle;
}

static void tile_texture(int fd,
                         const struct shaders * shaders,
                         int colorbuffer_handle,
                         int zbuffer_handle,
                         int flush_handle,
                         int texturebuffer_handle)
{
  int ib_dwords = _tile_texture(shaders,
                                TEXTUREBUFFER_RELOC_INDEX, // input
                                COLORBUFFER_RELOC_INDEX);  // output

  drm_radeon_cs(fd,
                colorbuffer_handle,
                zbuffer_handle,
                flush_handle,
                &texturebuffer_handle,
                1,
                ib_dwords);
}

static const char * vertex_shader_paths[] = {
  "texture_tile.vs.bin",
};
static const int vertex_shader_paths_length = (sizeof (vertex_shader_paths)) / (sizeof (vertex_shader_paths[0]));
static const char * fragment_shader_paths[] = {
  "texture_tile.fs.bin",
};
static const int fragment_shader_paths_length = (sizeof (fragment_shader_paths)) / (sizeof (fragment_shader_paths[0]));

int * load_textures_tiled(int fd,
                          int zbuffer_handle,
                          int flush_handle,
                          const char ** textures,
                          int textures_length,
                          int max_width,
                          int max_height)
{
  struct shaders shaders = {
    .vertex = load_shaders(vertex_shader_paths, vertex_shader_paths_length),
    .fragment = load_shaders(fragment_shader_paths, fragment_shader_paths_length),
    .vertex_length = vertex_shader_paths_length,
    .fragment_length = fragment_shader_paths_length,
  };

  // shaders
  load_pvs_shaders(shaders.vertex, shaders.vertex_length);
  load_us_shaders(shaders.fragment, shaders.fragment_length);

  int * texturebuffer_handle = (int *)malloc((sizeof (int)) * textures_length);

  int max_size = max_width * max_height * 4;
  void * temp_ptr;
  int temp_handle = create_buffer(fd, max_size, &temp_ptr);

  for (int i = 0; i < textures_length; i++) {
    int size = 0;
    void * buf = file_read(textures[i], &size);
    assert(buf != NULL);

    printf("load tiled texture[%d]: %d\n", i, size);

    for (int i = 0; i < size / 4; i++) {
      ((uint32_t*)temp_ptr)[i] = ((uint32_t*)buf)[i];
    }
    asm volatile ("" ::: "memory");
    free(buf);

    assert(size <= max_size);
    int handle = create_buffer(fd, size, NULL);

    tile_texture(fd,
                 &shaders,
                 handle,
                 zbuffer_handle,
                 flush_handle,
                 temp_handle);

    texturebuffer_handle[i] = handle;
  }
  sleep(1);

  munmap(temp_ptr, max_size);

  return texturebuffer_handle;
}
