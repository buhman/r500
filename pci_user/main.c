#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <math.h>

static inline uint32_t rreg(void * rmmio, uint32_t offset)
{
  uint32_t value = *((volatile uint32_t *)(((uintptr_t)rmmio) + offset));
  asm volatile ("" ::: "memory");
}

static inline uint32_t wreg(void * rmmio, uint32_t offset, uint32_t value)
{
  *((volatile uint32_t *)(((uintptr_t)rmmio) + offset)) = value;
  asm volatile ("" ::: "memory");
}

int main()
{
  ////////////////////////////////////////////////////////////////////////
  // PCI resource0
  ////////////////////////////////////////////////////////////////////////
  const char * resource2_path = "/sys/bus/pci/devices/0000:01:00.0/resource2";
  int resource2_fd = open(resource2_path, O_RDWR | O_SYNC);
  assert(resource2_fd >= 0);

  uint32_t resource2_size = 0x10000;
  void * resource2_base = mmap(0, resource2_size, PROT_READ | PROT_WRITE, MAP_SHARED, resource2_fd, 0);
  assert(resource2_base != MAP_FAILED);

  void * rmmio = resource2_base;

  uint32_t value1 = rreg(rmmio, 0x6110);
  printf("[r500] D1GRPH_PRIMARY_SURFACE_ADDRESS %08x\n", value1);
  uint32_t value2 = rreg(rmmio, 0x6110 + 0x800);
  printf("[r500] D2GRPH_PRIMARY_SURFACE_ADDRESS %08x\n", value2);

  uint32_t value3 = rreg(rmmio, 0x6118);
  printf("[r500] D1GRPH_SECONDARY_SURFACE_ADDRESS %08x\n", value3);
  uint32_t value4 = rreg(rmmio, 0x6118 + 0x800);
  printf("[r500] D2GRPH_SECONDARY_SURFACE_ADDRESS %08x\n", value4);

  wreg(rmmio, 0x6110, 0x813000);
  wreg(rmmio, 0x6118, 0x813000);
}
