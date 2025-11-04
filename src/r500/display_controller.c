#include <assert.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "registers.h"

void * map_pci_resource2()
{
  //////////////////////////////////////////////////////////////////////////////
  // PCI resource0
  //////////////////////////////////////////////////////////////////////////////
  const char * resource2_path = "/sys/bus/pci/devices/0000:01:00.0/resource2";
  int resource2_fd = open(resource2_path, O_RDWR | O_SYNC);
  assert(resource2_fd >= 0);

  uint32_t resource2_size = 0x10000;
  void * resource2_base = mmap(0, resource2_size, PROT_READ | PROT_WRITE, MAP_SHARED, resource2_fd, 0);
  assert(resource2_base != MAP_FAILED);

  void * rmmio = resource2_base;

  return rmmio;
}

void primary_surface_address(void * rmmio, int colorbuffer_ix)
{
#define D1CRTC_DOUBLE_BUFFER_CONTROL 0x60ec
#define D1GRPH_PRIMARY_SURFACE_ADDRESS 0x6110
#define D1GRPH_UPDATE 0x6144
#define D1GRPH_UPDATE__D1GRPH_SURFACE_UPDATE_PENDING (1 << 2)

  uint32_t d1crtc_double_buffer_control = rreg(rmmio, D1CRTC_DOUBLE_BUFFER_CONTROL);
  //printf("D1CRTC_DOUBLE_BUFFER_CONTROL: %08x\n", d1crtc_double_buffer_control);
  assert(d1crtc_double_buffer_control == (1 << 8));

  // addresses were retrieved from /sys/kernel/debug/radeon_vram_mm
  //
  // This assumes GEM buffer allocation always starts from the lowest
  // unallocated address.
  const uint32_t colorbuffer_addresses[2] = {
    0x813000,
    0xf66000,
  };

  wreg(rmmio, D1GRPH_PRIMARY_SURFACE_ADDRESS, colorbuffer_addresses[colorbuffer_ix]);
  while ((rreg(rmmio, D1GRPH_UPDATE) & D1GRPH_UPDATE__D1GRPH_SURFACE_UPDATE_PENDING) != 0);
}
