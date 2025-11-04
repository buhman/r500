#pragma once

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
