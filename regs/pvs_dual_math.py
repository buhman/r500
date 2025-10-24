def SRC_REG_TYPE(n):
    return (n >> 0) & 0x3

def SRC_REG_TYPE_gen(n):
    assert (0x3 & n) == n, (n, 0x3)
    return n << 0

def DST_OPCODE_MSB(n):
    return (n >> 2) & 0x1

def DST_OPCODE_MSB_gen(n):
    assert (0x1 & n) == n, (n, 0x1)
    return n << 2

def SRC_ABS_XYZW(n):
    return (n >> 3) & 0x1

def SRC_ABS_XYZW_gen(n):
    assert (0x1 & n) == n, (n, 0x1)
    return n << 3

def SRC_ADDR_MODE_0(n):
    return (n >> 4) & 0x1

def SRC_ADDR_MODE_0_gen(n):
    assert (0x1 & n) == n, (n, 0x1)
    return n << 4

def SRC_OFFSET(n):
    return (n >> 5) & 0xff

def SRC_OFFSET_gen(n):
    assert (0xff & n) == n, (n, 0xff)
    return n << 5

def SRC_SWIZZLE_X(n):
    return (n >> 13) & 0x7

def SRC_SWIZZLE_X_gen(n):
    assert (0x7 & n) == n, (n, 0x7)
    return n << 13

def SRC_SWIZZLE_Y(n):
    return (n >> 16) & 0x7

def SRC_SWIZZLE_Y_gen(n):
    assert (0x7 & n) == n, (n, 0x7)
    return n << 16

def DUAL_MATH_DST_OFFSET(n):
    return (n >> 19) & 0x3

def DUAL_MATH_DST_OFFSET_gen(n):
    assert (0x3 & n) == n, (n, 0x3)
    return n << 19

def DST_OPCODE(n):
    return (n >> 21) & 0xf

def DST_OPCODE_gen(n):
    assert (0xf & n) == n, (n, 0xf)
    return n << 21

def SRC_MODIFIER_X(n):
    return (n >> 25) & 0x1

def SRC_MODIFIER_X_gen(n):
    assert (0x1 & n) == n, (n, 0x1)
    return n << 25

def SRC_MODIFIER_Y(n):
    return (n >> 26) & 0x1

def SRC_MODIFIER_Y_gen(n):
    assert (0x1 & n) == n, (n, 0x1)
    return n << 26

def DST_WE_SEL(n):
    return (n >> 27) & 0x3

def DST_WE_SEL_gen(n):
    assert (0x3 & n) == n, (n, 0x3)
    return n << 27

def SRC_ADDR_SEL(n):
    return (n >> 29) & 0x3

def SRC_ADDR_SEL_gen(n):
    assert (0x3 & n) == n, (n, 0x3)
    return n << 29

def SRC_ADDR_MODE_1(n):
    return (n >> 31) & 0x1

def SRC_ADDR_MODE_1_gen(n):
    assert (0x1 & n) == n, (n, 0x1)
    return n << 31

table = [
    ("SRC_REG_TYPE", SRC_REG_TYPE),
    ("DST_OPCODE_MSB", DST_OPCODE_MSB),
    ("SRC_ABS_XYZW", SRC_ABS_XYZW),
    ("SRC_ADDR_MODE_0", SRC_ADDR_MODE_0),
    ("SRC_OFFSET", SRC_OFFSET),
    ("SRC_SWIZZLE_X", SRC_SWIZZLE_X),
    ("SRC_SWIZZLE_Y", SRC_SWIZZLE_Y),
    ("DUAL_MATH_DST_OFFSET", DUAL_MATH_DST_OFFSET),
    ("DST_OPCODE", DST_OPCODE),
    ("SRC_MODIFIER_X", SRC_MODIFIER_X),
    ("SRC_MODIFIER_Y", SRC_MODIFIER_Y),
    ("DST_WE_SEL", DST_WE_SEL),
    ("SRC_ADDR_SEL", SRC_ADDR_SEL),
    ("SRC_ADDR_MODE_1", SRC_ADDR_MODE_1),
]
