#pragma once

#define COLORBUFFER_RELOC_INDEX 0
#define ZBUFFER_RELOC_INDEX 1
//#define FLUSH_RELOC_INDEX 2
#define VERTEXBUFFER_RELOC_INDEX 2
#define TEXTUREBUFFER_RELOC_INDEX 3

#ifdef __cplusplus
extern "C" {
#endif

int drm_radeon_cs(int fd,
                  int colorbuffer_handle,
                  int zbuffer_handle,
                  int vertexbuffer_handle,
                  int * texturebuffer_handles,
                  int texturebuffer_handles_length,
                  int ib_dwords);

#ifdef __cplusplus
}
#endif
