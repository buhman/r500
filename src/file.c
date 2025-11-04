#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "file.h"

void * file_read(const char * path, int * size_out)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
    return NULL;
  }

  off_t size = lseek(fd, 0, SEEK_END);
  if (size == (off_t)-1) {
    fprintf(stderr, "lseek(%s, SEEK_END): %s\n", path, strerror(errno));
    return NULL;
  }

  off_t start = lseek(fd, 0, SEEK_SET);
  if (start == (off_t)-1) {
    fprintf(stderr, "lseek(%s, SEEK_SET): %s\n", path, strerror(errno));
    return NULL;
  }

  void * buf = malloc(size);

  ssize_t read_size = read(fd, buf, size);
  if (read_size == -1) {
    fprintf(stderr, "read(%s): %s\n", path, strerror(errno));
    return NULL;
  }

  int ret = close(fd);
  if (ret == -1) {
    fprintf(stderr, "close(%s): %s\n", path, strerror(errno));
    return NULL;
  }

  if (size_out != NULL) {
    *size_out = size;
  }

  return buf;
}
