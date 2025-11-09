#include <stdio.h>
#include <stdint.h>

#include <xf86drm.h>
#include <libdrm/radeon_drm.h>

#include "drm.h"
#include "../r500/indirect_buffer.h" // for extern uint32_t ib[];

int drm_radeon_cs(int fd,
                  int colorbuffer_handle,
                  int zbuffer_handle,
                  int vertexbuffer_handle,
                  int * texturebuffer_handles,
                  int texturebuffer_handles_length,
                  int ib_dwords)
{
  struct drm_radeon_cs_reloc relocs[3 + texturebuffer_handles_length];

  relocs[COLORBUFFER_RELOC_INDEX] = (struct drm_radeon_cs_reloc){
    .handle = colorbuffer_handle,
    .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
    .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
    .flags = 8,
  };
  relocs[ZBUFFER_RELOC_INDEX] = (struct drm_radeon_cs_reloc){
    .handle = zbuffer_handle,
    .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
    .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
    .flags = 8,
  };
  /*
  relocs[FLUSH_RELOC_INDEX] = (struct drm_radeon_cs_reloc){
    .handle = flush_handle,
    .read_domains = 2, // RADEON_GEM_DOMAIN_GTT
    .write_domain = 2, // RADEON_GEM_DOMAIN_GTT
    .flags = 0,
  };
  */
  relocs[VERTEXBUFFER_RELOC_INDEX] = (struct drm_radeon_cs_reloc){
    .handle = vertexbuffer_handle,
    .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
    .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
    .flags = 8,
  };

  for (int i = 0; i < texturebuffer_handles_length; i++) {
    relocs[TEXTUREBUFFER_RELOC_INDEX + i] = (struct drm_radeon_cs_reloc){
      .handle = texturebuffer_handles[i],
      .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
      .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
      .flags = 8,
    };
  }

  const uint32_t flags[2] = {
    5, // RADEON_CS_KEEP_TILING_FLAGS | RADEON_CS_END_OF_FRAME
    0, // RADEON_CS_RING_GFX
  };

  struct drm_radeon_cs_chunk chunks[3] = {
    {
      .chunk_id = RADEON_CHUNK_ID_IB,
      .length_dw = ib_dwords,
      .chunk_data = (uint64_t)(uintptr_t)ib,
    },
    {
      .chunk_id = RADEON_CHUNK_ID_RELOCS,
      .length_dw = (sizeof (relocs)) / (sizeof (uint32_t)),
      .chunk_data = (uint64_t)(uintptr_t)relocs,
    },
    {
      .chunk_id = RADEON_CHUNK_ID_FLAGS,
      .length_dw = (sizeof (flags)) / (sizeof (uint32_t)),
      .chunk_data = (uint64_t)(uintptr_t)&flags,
    },
  };

  uint64_t chunks_array[3] = {
    (uint64_t)(uintptr_t)&chunks[0],
    (uint64_t)(uintptr_t)&chunks[1],
    (uint64_t)(uintptr_t)&chunks[2],
  };

  struct drm_radeon_cs cs = {
    .num_chunks = 3,
    .cs_id = 0,
    .chunks = (uint64_t)(uintptr_t)chunks_array,
    .gart_limit = 0,
    .vram_limit = 0,
  };

  int ret = drmCommandWriteRead(fd, DRM_RADEON_CS, &cs, (sizeof (struct drm_radeon_cs)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_CS)");
    return -1;
  }

  return 0;
}
