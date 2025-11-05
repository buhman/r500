#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int * load_textures(int fd,
                    const char ** textures,
                    int textures_length);

int * load_textures_tiled(int fd,
                          int zbuffer_handle,
                          int flush_handle,
                          const char ** textures,
                          int textures_length,
                          int max_width,
                          int max_height);

#ifdef __cplusplus
}
#endif
