#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int create_buffer(int fd, int buffer_size, void ** out_ptr);
int create_flush_buffer(int fd);
int * load_textures(int fd,
                    const char ** textures,
                    int textures_length);

#ifdef __cplusplus
}
#endif
