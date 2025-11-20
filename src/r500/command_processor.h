#pragma once

#define _NOP 0x10
#define _3D_LOAD_VBPNTR 0x2f
#define _INDX_BUFFER 0x33
#define _3D_DRAW_VBUF_2 0x34
#define _3D_DRAW_IMMD_2 0x35
#define _3D_DRAW_INDX_2 0x36

#define TYPE_0_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_0_ONE_REG (1 << 15)
#define TYPE_0_BASE_INDEX(i) (((i) & 0x1fff) << 0)

#define TYPE_3_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_3_OPCODE(o) (((o) & 0xff) << 8)

#define INDX_BUFFER__0__ONE_REG_WR(n) (((n) & 1) << 31)
#define INDX_BUFFER__0__SKIP_COUNT(n) (((n) & 0x7) << 16)
#define INDX_BUFFER__0__DESTINATION(n) (((n) & 0x1fff) << 0)

#define INDX_BUFFER__1__BUFFER_BASE(n) ((n) << 0)

#define INDX_BUFFER__2__BUFFER_SIZE(n) ((n) << 0)
