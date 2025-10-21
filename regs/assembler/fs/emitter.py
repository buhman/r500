from os import path
from pprint import pprint
from functools import partial

import parse_bits
from assembler.fs.validator import SrcAddrType

class BaseRegister:
    def set(self, code, value, *, code_ix, descriptor):
        if type(descriptor.bits) is int:
            mask = 1
            low = descriptor.bits
        else:
            high, low = descriptor.bits
            assert high > low
            mask_length = (high - low) + 1
            mask = (1 << mask_length) - 1

        code_value = code[code_ix]
        assert (code_value >> low) & mask == 0
        assert value & mask == value
        code[code_ix] |= (value & mask) << low

_descriptor_indicies = {
    "US_CMN_INST": 0,
    "US_ALU_RGB_ADDR": 1,
    "US_ALU_ALPHA_ADDR": 2,
    "US_ALU_RGB_INST": 3,
    "US_ALU_ALPHA_INST": 4,
    "US_ALU_RGBA_INST": 5,

    "US_TEX_INST": 1,
    "US_TEX_ADDR": 2,
    "US_TEX_ADDR_DXDY": 3,

    "US_FC_INST": 2,
    "US_FC_ADDR": 3,
}

def parse_register(register_name):
    base = path.dirname(__file__)

    filename = path.join(base, "..", "..", "bits", register_name.lower() + ".txt")
    l = list(parse_bits.parse_file_fields(filename))
    cls = type(register_name, (BaseRegister,), {})
    instance = cls()
    descriptors = list(parse_bits.aggregate(l))
    code_ix = _descriptor_indicies[register_name]
    for descriptor in descriptors:
        setattr(instance, descriptor.field_name,
                partial(instance.set, code_ix=code_ix, descriptor=descriptor))
        func = getattr(instance, descriptor.field_name)
        for pv_value, (pv_name, _) in descriptor.possible_values.items():
            if pv_name is not None:
                setattr(func, pv_name, pv_value)
    assert getattr(instance, "descriptors", None) is None
    instance.descriptors = descriptors

    return instance

US_CMN_INST = parse_register("US_CMN_INST")
US_ALU_RGB_ADDR = parse_register("US_ALU_RGB_ADDR")
US_ALU_ALPHA_ADDR = parse_register("US_ALU_ALPHA_ADDR")
US_ALU_RGB_INST = parse_register("US_ALU_RGB_INST")
US_ALU_ALPHA_INST = parse_register("US_ALU_ALPHA_INST")
US_ALU_RGBA_INST = parse_register("US_ALU_RGBA_INST")
US_TEX_INST = parse_register("US_TEX_INST")
US_TEX_ADDR = parse_register("US_TEX_ADDR")
US_TEX_ADDR_DXDY = parse_register("US_TEX_ADDR_DXDY")
US_FC_INST = parse_register("US_FC_INST")
US_FC_ADDR = parse_register("US_FC_ADDR")

def emit_alpha_op(code, alpha_op):
    # dest
    if alpha_op.dest.wmask is not None:
        US_CMN_INST.ALPHA_WMASK(code, alpha_op.dest.wmask.value)
    if alpha_op.dest.omask is not None:
        US_CMN_INST.ALPHA_OMASK(code, alpha_op.dest.omask.value)

    # opcode
    US_ALU_ALPHA_INST.ALPHA_OP(code, alpha_op.opcode.value)

    # sels
    srcs = [
        US_ALU_ALPHA_INST.ALPHA_SEL_A,
        US_ALU_ALPHA_INST.ALPHA_SEL_B,
        US_ALU_RGBA_INST.ALPHA_SEL_C,
    ]
    swizzles = [
        [US_ALU_ALPHA_INST.ALPHA_SWIZ_A],
        [US_ALU_ALPHA_INST.ALPHA_SWIZ_B],
        [US_ALU_RGBA_INST.ALPHA_SWIZ_C],
    ]
    mods = [
        US_ALU_ALPHA_INST.ALPHA_MOD_A,
        US_ALU_ALPHA_INST.ALPHA_MOD_B,
        US_ALU_RGBA_INST.ALPHA_MOD_C,
    ]
    for sel, src_func, swizzle_funcs, mod_func in zip(alpha_op.sels,
                                                     srcs, swizzles, mods):
        src_func(code, sel.src.value)
        assert len(sel.swizzle) == 1
        assert len(swizzle_funcs) == 1
        for swizzle_func, swizzle in zip(swizzle_funcs, sel.swizzle):
            swizzle_func(code, swizzle.value)
        mod_func(code, sel.mod.value)

def emit_rgb_op(code, rgb_op):
    # dest
    if rgb_op.dest.wmask is not None:
        US_CMN_INST.RGB_WMASK(code, rgb_op.dest.wmask.value)
    if rgb_op.dest.omask is not None:
        US_CMN_INST.RGB_OMASK(code, rgb_op.dest.omask.value)

    # opcode
    US_ALU_RGBA_INST.RGB_OP(code, rgb_op.opcode.value)

    # sels
    srcs = [
        US_ALU_RGB_INST.RGB_SEL_A,
        US_ALU_RGB_INST.RGB_SEL_B,
        US_ALU_RGBA_INST.RGB_SEL_C,
    ]
    swizzles = [
        [US_ALU_RGB_INST.RED_SWIZ_A, US_ALU_RGB_INST.GREEN_SWIZ_A, US_ALU_RGB_INST.BLUE_SWIZ_A],
        [US_ALU_RGB_INST.RED_SWIZ_B, US_ALU_RGB_INST.GREEN_SWIZ_B, US_ALU_RGB_INST.BLUE_SWIZ_B],
        [US_ALU_RGBA_INST.RED_SWIZ_C, US_ALU_RGBA_INST.GREEN_SWIZ_C, US_ALU_RGBA_INST.BLUE_SWIZ_C],
    ]
    mods = [
        US_ALU_RGB_INST.RGB_MOD_A,
        US_ALU_RGB_INST.RGB_MOD_B,
        US_ALU_RGBA_INST.RGB_MOD_C,
    ]
    for sel, src_func, swizzle_funcs, mod_func in zip(rgb_op.sels,
                                                      srcs, swizzles, mods):
        src_func(code, sel.src.value)
        assert len(sel.swizzle) == 3
        assert len(swizzle_funcs) == 3
        for swizzle_func, swizzle in zip(swizzle_funcs, sel.swizzle):
            swizzle_func(code, swizzle.value)
        mod_func(code, sel.mod)

def emit_addr(code, addr):
    srcs = [
        (addr.alpha.src0 , US_ALU_ALPHA_ADDR.ADDR0 , US_ALU_ALPHA_ADDR.ADDR0_CONST),
        (addr.alpha.src1 , US_ALU_ALPHA_ADDR.ADDR1 , US_ALU_ALPHA_ADDR.ADDR1_CONST),
        (addr.alpha.src2 , US_ALU_ALPHA_ADDR.ADDR2 , US_ALU_ALPHA_ADDR.ADDR2_CONST),
        (addr.rgb.src0   , US_ALU_RGB_ADDR.ADDR0   , US_ALU_RGB_ADDR.ADDR0_CONST),
        (addr.rgb.src1   , US_ALU_RGB_ADDR.ADDR1   , US_ALU_RGB_ADDR.ADDR1_CONST),
        (addr.rgb.src2   , US_ALU_RGB_ADDR.ADDR2   , US_ALU_RGB_ADDR.ADDR2_CONST),
    ]

    for src, ADDR, ADDR_CONST in srcs:
        if src is not None:
            is_const = int(src.type is SrcAddrType.const)
            is_float = int(src.type is SrcAddrType.float)
            ADDR(code, (is_float << 7) | src.value)
            ADDR_CONST(code, is_const)
        else:
            ADDR(code, (1 << 7) | 0)

    if addr.alpha.srcp is not None:
        US_ALU_ALPHA_ADDR.SRCP_OP(code, addr.alpha.srcp.value)
    if addr.rgb.srcp is not None:
        US_ALU_RGB_ADDR.SRCP_OP(code, addr.rgb.srcp.value)

def emit_instruction(code, ins):
    US_CMN_INST.TYPE(code, ins.type.value)
    US_CMN_INST.TEX_SEM_WAIT(code, int(ins.tex_sem_wait))

    emit_addr(code, ins.addr)
    if ins.alpha_op is not None:
        emit_alpha_op(code, ins.alpha_op)
    if ins.rgb_op is not None:
        emit_rgb_op(code, ins.rgb_op)
