#pragma once

#define TYPE_0_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_0_ONE_REG (1 << 15)
#define TYPE_0_BASE_INDEX(i) (((i) & 0x1fff) << 0)

#define TYPE_3_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_3_OPCODE(o) (((o) & 0xff) << 8)

#define T0(address, count)                                              \
  do {                                                                  \
    ib[ix++].u32 = TYPE_0_COUNT(count) | TYPE_0_BASE_INDEX(address >> 2);   \
  } while (0);

#define T0_ONE_REG(address, count)                                      \
  do {                                                                  \
    ib[ix++].u32 = TYPE_0_COUNT(count) | TYPE_0_ONE_REG | TYPE_0_BASE_INDEX(address >> 2); \
  } while (0);

#define T0V(address, value)                                             \
  do {                                                                  \
    ib[ix++].u32 = TYPE_0_COUNT(0) | TYPE_0_BASE_INDEX(address >> 2);   \
    ib[ix++].u32 = value;                                               \
  } while (0);

#define T0Vf(address, value)                                            \
  do {                                                                  \
    ib[ix++].u32 = TYPE_0_COUNT(0) | TYPE_0_BASE_INDEX(address >> 2);   \
    ib[ix++].f32 = value;                                               \
  } while (0);

#define T3(opcode, count)                                               \
  do {                                                                  \
    ib[ix++].u32 = (0b11 << 30) | TYPE_3_COUNT(count) | TYPE_3_OPCODE(opcode); \
  } while (0);

#define _3D_DRAW_IMMD_2 0x35
