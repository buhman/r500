#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int create_buffer(int fd, int buffer_size, void ** out_ptr);
int create_flush_buffer(int fd);

#ifdef __cplusplus
}
#endif
