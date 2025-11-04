#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>

#include <xf86drm.h>
#include <libdrm/radeon_drm.h>

#include "buffer.h"
#include "../file.h"

int create_buffer(int fd, int buffer_size, void ** out_ptr)
{
  int ret;

  struct drm_radeon_gem_create args = {
    .size = buffer_size,
    .alignment = 4096,
    .handle = 0,
    .initial_domain = 4, // RADEON_GEM_DOMAIN_VRAM
    .flags = 4
  };

  ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_CREATE, &args, (sizeof (struct drm_radeon_gem_create)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_GEM_CREATE)");
  }
  assert(args.handle != 0);

  struct drm_radeon_gem_mmap mmap_args = {
    .handle = args.handle,
    .offset = 0,
    .size = buffer_size,
  };
  ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_MMAP, &mmap_args, (sizeof (struct drm_radeon_gem_mmap)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_GEM_MMAP)");
  }

  if (out_ptr != NULL) {
    void * ptr = mmap(0,
                      buffer_size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      fd,
                      mmap_args.addr_ptr);
    assert(ptr != MAP_FAILED);

    *out_ptr = ptr;
  }

  return args.handle;
}

int create_flush_buffer(int fd)
{
  int ret;

  struct drm_radeon_gem_create args = {
    .size = 4096,
    .alignment = 4096,
    .handle = 0,
    .initial_domain = 2, // GTT
    .flags = 0
  };

  ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_CREATE,
                            &args, (sizeof (args)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_GEM_CREATE)");
  }
  assert(args.handle != 0);
  return args.handle;
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
