#pragma once

#include <stdint.h>

#include "command_processor.h"
#include "shader.h"

#define T0(address, count)                                              \
  do {                                                                  \
    ib[ib_ix++].u32 = TYPE_0_COUNT(count) | TYPE_0_BASE_INDEX(address >> 2);   \
  } while (0);

#define T0_ONE_REG(address, count)                                      \
  do {                                                                  \
    ib[ib_ix++].u32 = TYPE_0_COUNT(count) | TYPE_0_ONE_REG | TYPE_0_BASE_INDEX(address >> 2); \
  } while (0);

#define T0V(address, value)                                             \
  do {                                                                  \
    ib[ib_ix++].u32 = TYPE_0_COUNT(0) | TYPE_0_BASE_INDEX(address >> 2);   \
    ib[ib_ix++].u32 = value;                                               \
  } while (0);

#define T0Vf(address, value)                                            \
  do {                                                                  \
    ib[ib_ix++].u32 = TYPE_0_COUNT(0) | TYPE_0_BASE_INDEX(address >> 2);   \
    ib[ib_ix++].f32 = value;                                               \
  } while (0);

#define T3(opcode, count)                                               \
  do {                                                                  \
    ib[ib_ix++].u32 = (0b11 << 30) | TYPE_3_COUNT(count) | TYPE_3_OPCODE(opcode); \
  } while (0);

#define TU(data)                                \
  do {                                          \
    ib[ib_ix++].u32 = data;                     \
  } while (0);

#define TF(data)                                \
  do {                                          \
    ib[ib_ix++].f32 = data;                     \
  } while (0);

#ifdef __cplusplus
extern "C" {
#endif

union u32_f32 {
  uint32_t u32;
  float f32;
};

extern union u32_f32 ib[16384];
extern volatile int ib_ix;

void ib_generic_initialization();
void ib_colorbuffer(int reloc_index);
void ib_zbuffer(int reloc_index, int zfunc);
void ib_rs_instructions(int count);
void ib_texture__0();
void ib_texture__1(int reloc_index);
void ib_vap_pvs(struct shader_offset * offset);
void ib_ga_us(struct shader_offset * offset);
void ib_vap_pvs_const_cntl(const float * consts, int size);
void ib_vap_stream_cntl__2();
void ib_vap_stream_cntl__323();

#ifdef __cplusplus
}
#endif
