import pvs_src
import pvs_src_bits
import pvs_dst
import pvs_dst_bits
import pvs_dual_math
import itertools
from functools import partial
import sys

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
    ("FRACTION", "FRC"),
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
    ve_sat = pvs_dst.VE_SAT(dst_op)
    me_sat = pvs_dst.ME_SAT(dst_op)
    pred_enable = pvs_dst.PRED_ENABLE(dst_op)
    pred_sense = pvs_dst.PRED_SENSE(dst_op)
    dual_math_op = pvs_dst.DUAL_MATH_OP(dst_op)
    addr_sel = pvs_dst.ADDR_SEL(dst_op)

    assert addr_mode == 0
    assert pred_enable == 0
    assert pred_sense == 0
    assert addr_sel == 0

    parts = []

    reg_str = pvs_dst_bits.PVS_DST_REG[reg_type]
    reg_str = reg_str.replace("TEMPORARY", "TEMP").lower()
    we_swizzle = dst_swizzle_from_we(dst_op)
    parts.append(f"{reg_str}[{offset}].{we_swizzle}")

    def get_opcode_str():
        if math_inst:
            assert not macro_inst
            return op_substitutions(pvs_dst_bits.MATH_OPCODE[opcode])
        elif macro_inst:
            return op_substitutions(pvs_dst_bits.MACRO_OPCODE[opcode])
        else:
            return op_substitutions(pvs_dst_bits.VECTOR_OPCODE[opcode])

    opcode_str = get_opcode_str()
    if ve_sat:
        assert not math_inst
        opcode_str += '.SAT'
    if me_sat:
        assert math_inst
        opcode_str += '.SAT'
    parts.append(opcode_str)

    return parts

def src_swizzle_from_src_op(swizzles, modifiers):
    modifiers = [
        '' if modifier == 0 else '-'
        for modifier in modifiers
    ]
    src_swizzle_select = [
        'x', 'y', 'z', 'w', '0', '1', 'h', '_'
    ]
    swizzles = [
        src_swizzle_select[swizzle]
        for swizzle in swizzles
    ]

    return ''.join(map(''.join, zip(modifiers, swizzles)))

def reg_type_str_from_reg_type(reg_type):
    reg_type_str = pvs_src_bits.PVS_SRC_REG_TYPE[reg_type]
    reg_type_str = reg_type_str.removeprefix("PVS_SRC_REG_")
    reg_type_str = reg_type_str.replace("TEMPORARY", "TEMP")
    reg_type_str = reg_type_str.replace("CONSTANT", "CONST")
    reg_type_str = reg_type_str.lower()
    return reg_type_str

def parse_src_op(src_op):
    reg_type = pvs_src.REG_TYPE(src_op)
    abs_xyzw = pvs_src.ABS_XYZW(src_op)
    addr_mode = ( (pvs_src.ADDR_MODE_1(src_op) << 1)
                | (pvs_src.ADDR_MODE_0(src_op) << 0))
    offset = pvs_src.OFFSET(src_op)
    addr_sel = pvs_src.ADDR_SEL(src_op)

    assert addr_mode == 0
    assert addr_sel == 0

    reg_type_str = reg_type_str_from_reg_type(reg_type)

    swizzle_x = pvs_src.SWIZZLE_X(src_op)
    swizzle_y = pvs_src.SWIZZLE_Y(src_op)
    swizzle_z = pvs_src.SWIZZLE_Z(src_op)
    swizzle_w = pvs_src.SWIZZLE_W(src_op)
    modifier_x = pvs_src.MODIFIER_X(src_op)
    modifier_y = pvs_src.MODIFIER_Y(src_op)
    modifier_z = pvs_src.MODIFIER_Z(src_op)
    modifier_w = pvs_src.MODIFIER_W(src_op)
    swizzles = [swizzle_x, swizzle_y, swizzle_z, swizzle_w]
    modifiers = [modifier_x, modifier_y, modifier_z, modifier_w]
    src_swizzle = src_swizzle_from_src_op(swizzles, modifiers)

    s = f"{reg_type_str}[{offset}].{src_swizzle}"
    if abs_xyzw:
        return f"|{s}|"
    else:
        return s

def parse_dual_math_instruction(instruction):
    src_reg_type = pvs_dual_math.SRC_REG_TYPE(instruction)
    dst_opcode = ( (pvs_dual_math.DST_OPCODE_MSB(instruction) << 4)
                 | (pvs_dual_math.DST_OPCODE(instruction) << 0) )
    src_abs = pvs_dual_math.SRC_ABS_XYZW(instruction)
    src_addr_mode = ( (pvs_dual_math.SRC_ADDR_MODE_1(instruction) << 1)
                    | (pvs_dual_math.SRC_ADDR_MODE_0(instruction) << 0))
    src_offset = pvs_dual_math.SRC_OFFSET(instruction)
    src_swizzle_x = pvs_dual_math.SRC_SWIZZLE_X(instruction)
    src_swizzle_y = pvs_dual_math.SRC_SWIZZLE_Y(instruction)
    dual_math_dst_offset = pvs_dual_math.DUAL_MATH_DST_OFFSET(instruction)
    src_modifier_x = pvs_dual_math.SRC_MODIFIER_X(instruction)
    src_modifier_y = pvs_dual_math.SRC_MODIFIER_Y(instruction)
    dst_we_sel = pvs_dual_math.DST_WE_SEL(instruction)
    src_addr_sel = pvs_dual_math.SRC_ADDR_SEL(instruction)

    assert src_addr_sel == 0
    assert src_addr_mode == 0

    we_str = ["x", "y", "z", "w"][dst_we_sel]
    dst_str = f"alt_temp[{dual_math_dst_offset}].{we_str}"

    opcode_str = op_substitutions(pvs_dst_bits.MATH_OPCODE[dst_opcode])

    swizzles = [src_swizzle_x, src_swizzle_y]
    modifiers = [src_modifier_x, src_modifier_y]
    src_swizzle = src_swizzle_from_src_op(swizzles, modifiers)

    reg_type_str = reg_type_str_from_reg_type(src_reg_type)

    src_str = f"{reg_type_str}[{src_offset}].{src_swizzle}"

    if src_abs:
        src_str = f"|{src}|"

    return dst_str, opcode_str, src_str

def parse_instruction(instruction):
    dst_op = instruction[0]
    src_op0 = instruction[1]
    src_op1 = instruction[2]
    src_op2 = instruction[3]

    dual_math_op = pvs_dst.DUAL_MATH_OP(dst_op)
    if dual_math_op == 0:
        dst, op, *rest = itertools.chain(
            parse_dst_op(dst_op),
            [
                parse_src_op(src_op0),
                parse_src_op(src_op1),
                parse_src_op(src_op2),
            ]
        )
        print(dst.ljust(15), "=", op.ljust(12), " ".join(map(lambda s: s.ljust(17), rest)).rstrip(), ';')
    else:
        dst, op, *rest = itertools.chain(
            parse_dst_op(dst_op),
            [
                parse_src_op(src_op0),
                parse_src_op(src_op1),
            ]
        )
        print(dst.ljust(15), "=", op.ljust(12), " ".join(map(lambda s: s.ljust(17), rest)).rstrip(), ',')
        dm_dst, dm_op, dm_src = parse_dual_math_instruction(src_op2)
        print(dm_dst.ljust(15), "=", dm_op.ljust(12), dm_src, ';')

def parse_hex(s):
    assert s.startswith('0x')
    return int(s.removeprefix('0x'), 16)

if __name__ == "__main__":
    filename = sys.argv[1]
    with open(filename) as f:
        buf = f.read()
    code = [parse_hex(c.strip()) for c in buf.split(',') if c.strip()]
    for i in range(len(code) // 4):
        parse_instruction(code[i*4:i*4+4])
