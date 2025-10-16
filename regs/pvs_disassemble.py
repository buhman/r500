import pvs_src
import pvs_src_bits
import pvs_dst
import pvs_dst_bits
from pprint import pprint
import itertools
from functools import partial

code = [
    0x00f00203,
    0x00d10001,
    0x01248001,
    0x01248001,
]

# Radeon Compiler Program
#  0: MOV output[1].xyz, input[1].xyz_;
#  1: MOV output[0], input[0].xyz1;
# Final vertex program code:
# 0: op: 0x00702203 dst: 1o op:                    VE_ADD
#  src0: 0x01d10021 reg: 1i swiz:  X/ Y/ Z/ U
#  src1: 0x01248021 reg: 1i swiz:  0/ 0/ 0/ 0
#  src2: 0x01248021 reg: 1i swiz:  0/ 0/ 0/ 0
# 1: op: 0x00f00203 dst: 0o op:                    VE_ADD
#  src0: 0x01510001 reg: 0i swiz:  X/ Y/ Z/ 1
#  src1: 0x01248001 reg: 0i swiz:  0/ 0/ 0/ 0
#  src2: 0x01248001 reg: 0i swiz:  0/ 0/ 0/ 0
code = [
    0x00702203,
    0x01d10021,
    0x01248021,
    0x01248021,
    0x00f00203,
    0x01510001,
    0x01248001,
    0x01248001,
]

code = [
    0x00f00003,
    0x00d10022,
    0x01248022,
    0x01248022,
    0x00f02003,
    0x00d10022,
    0x01248022,
    0x01248022,
    0x00100004,
    0x01ff0002,
    0x01ff0020,
    0x01ff2000,
    0x00100006,
    0x01ff0000,
    0x01248000,
    0x01248000,
    0x00100004,
    0x01ff0000,
    0x01ff4022,
    0x01ff6022,
    0x00100050,
    0x00000000,
    0x01248000,
    0x01248000,
    0x00f00204,
    0x0165a000,
    0x01690001,
    0x01240000,
]

code = [
    0x00f00003,
    0x00d10022,
    0x01248022,
    0x01248022,
    0x00f02003,
    0x00d10022,
    0x01248022,
    0x01248022,
    0x00100004,
    0x01ff0002,
    0x01ff0020,
    0x01ff2000,
    0x00100006,
    0x01ff0000,
    0x01248000,
    0x01248000,
    0x00100004,
    0x01ff0000,
    0x01ff4022,
    0x01ff6022,
    0x00200051,
    0x00000000,
    0x01248000,
    0x01248000,
    0x00100050,
    0x00000000,
    0x01248000,
    0x01248000,
    0x00600002,
    0x01c8e001,
    0x01c9e000,
    0x01248000,
    0x00500204,
    0x1fe72001,
    0x01e70000,
    0x01e72000,
    0x00a00204,
    0x0138e001,
    0x0138e000,
    0x017ae000,
]

def out(level, *args):
    sys.stdout.write("    " * level + " ".join(args))

def parse_code(code):
    ix = 0

    pvs_dst_maxlen = max(len(k) for k, _ in  pvs_dst.table)
    print(pvs_dst_maxlen)

    while ix < len(code):
        print(f"ix: {ix // 4}")
        dst_op = code[ix + 0]
        src_op0 = code[ix + 1]
        src_op1 = code[ix + 2]
        src_op2 = code[ix + 3]
        print(f"  dst: {dst_op:08x}")
        for name, parse in pvs_dst.table:
            namepad = name.ljust(pvs_dst_maxlen + 1)
            value = parse(dst_op)
            if name == "OPCODE":
                if pvs_dst.MATH_INST(dst_op):
                    print("   ", namepad, pvs_dst_bits.MATH_OPCODE[value])
                else:
                    print("   ", namepad, pvs_dst_bits.VECTOR_OPCODE[value])
            elif name == "REG_TYPE":
                print("   ", namepad, pvs_dst_bits.PVS_DST_REG[value])
            else:
                print("   ", namepad, value)

        print(f"  src0: {src_op0:08x}")
        for name, parse in pvs_src.table:
            print("   ", name, parse(src_op0))
        print(f"  src1: {src_op1:08x}")
        for name, parse in pvs_src.table:
            print("   ", name, parse(src_op1))
        print(f"  src2: {src_op2:08x}")
        for name, parse in pvs_src.table:
            print("   ", name, parse(src_op2))

        ix += 4

#parse_code(code)

def dst_swizzle_from_we(dst_op):
    table = [
        (pvs_dst.WE_X, "x"),
        (pvs_dst.WE_Y, "y"),
        (pvs_dst.WE_Z, "z"),
        (pvs_dst.WE_W, "w"),
    ]
    return ''.join(
        c for func, c in table
        if func(dst_op)
    )

_op_substitutions = [
    ("DOT_PRODUCT", "DOT"),
    ("MULTIPLY_ADD", "MAD"),
    ("FRACTION", "FRAC"),
    ("MULTIPLY", "MUL"),
    ("MAXMIUM", "MAX"),
    ("MINIMUM", "MIN"),
]

def op_substitutions(s):
    for k, v in _op_substitutions:
        if k in s:
            s = s.replace(k, v)
    return s

def parse_dst_op(dst_op):
    opcode = pvs_dst.OPCODE(dst_op)
    math_inst = pvs_dst.MATH_INST(dst_op)
    macro_inst = pvs_dst.MACRO_INST(dst_op)
    reg_type = pvs_dst.REG_TYPE(dst_op)
    addr_mode = ( (pvs_dst.ADDR_MODE_1(dst_op) << 1)
                | (pvs_dst.ADDR_MODE_0(dst_op) << 0))
    offset = pvs_dst.OFFSET(dst_op)
    pred_enable = pvs_dst.PRED_ENABLE(dst_op)
    pred_sense = pvs_dst.PRED_SENSE(dst_op)
    dual_math_op = pvs_dst.DUAL_MATH_OP(dst_op)
    addr_sel = pvs_dst.ADDR_SEL(dst_op)

    assert addr_mode == 0
    assert macro_inst == 0
    assert pred_enable == 0
    assert pred_sense == 0
    assert dual_math_op == 0
    assert addr_sel == 0

    parts = []

    reg_str = pvs_dst_bits.PVS_DST_REG[reg_type]
    reg_str = reg_str.replace("TEMPORARY", "TEMP").lower()
    we_swizzle = dst_swizzle_from_we(dst_op)
    parts.append(f"{reg_str}[{offset}].{we_swizzle}")

    if math_inst:
        parts.append(op_substitutions(pvs_dst_bits.MATH_OPCODE[opcode]))
    else:
        parts.append(op_substitutions(pvs_dst_bits.VECTOR_OPCODE[opcode]))

    return parts

def src_swizzle_from_src_op(src_op):
    swizzle_x = pvs_src.SWIZZLE_X(src_op)
    swizzle_y = pvs_src.SWIZZLE_Y(src_op)
    swizzle_z = pvs_src.SWIZZLE_Z(src_op)
    swizzle_w = pvs_src.SWIZZLE_W(src_op)
    modifier_x = pvs_src.MODIFIER_X(src_op)
    modifier_y = pvs_src.MODIFIER_Y(src_op)
    modifier_z = pvs_src.MODIFIER_Z(src_op)
    modifier_w = pvs_src.MODIFIER_W(src_op)

    modifiers = [
        '' if modifier == 0 else '-'
        for modifier
        in [modifier_x, modifier_y, modifier_z, modifier_w]
    ]
    src_swizzle_select = [
        'x', 'y', 'z', 'w', '0', '1', 'h', '_'
    ]
    swizzles = [
        src_swizzle_select[swizzle]
        for swizzle
        in [swizzle_x, swizzle_y, swizzle_z, swizzle_w]
    ]

    return ''.join(map(''.join, zip(modifiers, swizzles)))

def parse_src_op(src_op):
    reg_type = pvs_src.REG_TYPE(src_op)
    abs_xyzw = pvs_src.ABS_XYZW(src_op)
    addr_mode = ( (pvs_src.ADDR_MODE_1(src_op) << 1)
                | (pvs_src.ADDR_MODE_0(src_op) << 0))
    offset = pvs_src.OFFSET(src_op)
    addr_sel = pvs_src.ADDR_SEL(src_op)

    assert addr_mode == 0
    assert addr_sel == 0

    reg_type_str = pvs_src_bits.PVS_SRC_REG_TYPE[reg_type]
    reg_type_str = reg_type_str.removeprefix("PVS_SRC_REG_")
    reg_type_str = reg_type_str.replace("TEMPORARY", "TEMP")
    reg_type_str = reg_type_str.replace("CONSTANT", "CONST")
    reg_type_str = reg_type_str.lower()

    src_swizzle = src_swizzle_from_src_op(src_op)
    if abs_xyzw:
        src_swizzle = f"abs({src_swizzle})"

    return f"{reg_type_str}[{offset}].{src_swizzle}"

def parse_instruction(instruction):
    dst_op = instruction[0]
    src_op0 = instruction[1]
    src_op1 = instruction[2]
    src_op2 = instruction[3]

    dst, op, *rest = itertools.chain(
        parse_dst_op(dst_op),
        [
            parse_src_op(src_op0),
            parse_src_op(src_op1),
            parse_src_op(src_op2),
        ]
    )

    print(dst.ljust(12), "=", op.ljust(9), " ".join(map(lambda s: s.ljust(17), rest)))

for i in range(len(code) // 4):
    parse_instruction(code[i*4:i*4+4])
