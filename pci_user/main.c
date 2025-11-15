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
  return value;
}

static inline void wreg(void * rmmio, uint32_t offset, uint32_t value)
{
  *((volatile uint32_t *)(((uintptr_t)rmmio) + offset)) = value;
  asm volatile ("" ::: "memory");
}

static inline void wreg_slow(void * rmmio, uint32_t offset, uint32_t value)
{
  #define MM_INDEX 0x0
  #define MM_DATA 0x4
  wreg(rmmio, MM_INDEX, offset);
  wreg(rmmio, MM_DATA, value);
}

static inline uint32_t rreg_slow(void * rmmio, uint32_t offset)
{
  wreg(rmmio, MM_INDEX, offset);
  uint32_t value = rreg(rmmio, MM_DATA);
  return value;
}

struct name_address {
  const char * name;
  const int address;
};

const struct name_address display_addresses[] = {
  #include "../regs/display_registers.inc"
};
const int display_addresses_length = (sizeof (display_addresses)) / (sizeof (display_addresses[0]));

const struct name_address memory_controller_addresses[] = {
  #include "../regs/memory_controller_registers.inc"
};
const int memory_controller_addresses_length = (sizeof (memory_controller_addresses)) / (sizeof (memory_controller_addresses[0]));

const struct name_address pcie_addresses[] = {
  #include "../regs/pcie_registers.inc"
};
const int pcie_addresses_length = (sizeof (pcie_addresses)) / (sizeof (pcie_addresses[0]));

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

  if (1) {
    for (int i = 0; i < display_addresses_length; i++) {
      uint32_t value = rreg(rmmio, display_addresses[i].address);
      printf("%s %x %08x\n", display_addresses[i].name, display_addresses[i].address, value);
    }
  }

  if (0) {
#define MC_IND_INDEX 0x70
#define MC_IND_INDEX__MC_IND_ADDR(x) (((x) & 0xffff) << 0)
#define MC_IND_INDEX__MC_IND_ADDR__CLEAR (~0xffff)
#define MC_IND_DATA 0x74
    // skip MC_IND_INDEX/MC_IND_DATA
    const int masks[] = {
      (1 << 16),
      (1 << 17),
      (1 << 20),
      (1 << 21),
      (1 << 22),
    };

    for (int i = 2; i < memory_controller_addresses_length; i++) {
      const char * name = memory_controller_addresses[i].name;
      int address = memory_controller_addresses[i].address;
      int mask = (1 << 16) | (1 << 17) | (1 << 20) | (1 << 21) | (1 << 22);
      wreg(rmmio, MC_IND_INDEX, MC_IND_INDEX__MC_IND_ADDR(address) | mask);
      uint32_t value = rreg(rmmio, MC_IND_DATA);
      wreg(rmmio, MC_IND_INDEX, MC_IND_INDEX__MC_IND_ADDR__CLEAR);
      printf("%s %x %08x\n", name, address, value);
    }
  }

  if (0) {
#define PCIE_INDEX 0x30
#define PCIE_DATA 0x38
    for (int i = 0; i < pcie_addresses_length; i++) {
      const char * name = pcie_addresses[i].name;
      int address = pcie_addresses[i].address;
      wreg_slow(rmmio, PCIE_INDEX, address);
      uint32_t value = rreg_slow(rmmio, PCIE_DATA);
      printf("%s %x %08x\n", name, address, value);
    }
  }
}
