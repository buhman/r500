#pragma once

#define _NOP 0x10
#define _3D_LOAD_VBPNTR 0x2f
#define _3D_DRAW_VBUF_2 0x34
#define _3D_DRAW_IMMD_2 0x35

#define TYPE_0_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_0_ONE_REG (1 << 15)
#define TYPE_0_BASE_INDEX(i) (((i) & 0x1fff) << 0)

#define TYPE_3_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_3_OPCODE(o) (((o) & 0xff) << 8)
