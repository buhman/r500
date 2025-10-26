from assembler.fs.keywords import KW
from assembler.fs.alu_validator import SrcAddrType, InstructionType
from assembler.fs.common_emitter import US_CMN_INST, US_ALU_RGB_ADDR, US_ALU_ALPHA_ADDR
from assembler.fs.common_emitter import US_ALU_RGB_INST, US_ALU_ALPHA_INST, US_ALU_RGBA_INST

def emit_alpha_op(code, alpha_op):
    # dest
    if alpha_op.dest.wmask is not None:
        US_CMN_INST.ALPHA_WMASK(code, alpha_op.dest.wmask.value)
    if alpha_op.dest.omask is not None:
        US_CMN_INST.ALPHA_OMASK(code, alpha_op.dest.omask.value)
    assert type(alpha_op.dest.addrd) is int
    US_ALU_ALPHA_INST.ALPHA_ADDRD(code, alpha_op.dest.addrd)

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
    assert type(rgb_op.dest.addrd) is int
    US_ALU_RGBA_INST.RGB_ADDRD(code, rgb_op.dest.addrd)

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
    US_CMN_INST.TYPE(code, InstructionType.OUT if KW.OUT in ins.tags else InstructionType.ALU)
    US_CMN_INST.TEX_SEM_WAIT(code, int(KW.TEX_SEM_WAIT in ins.tags))
    US_CMN_INST.LAST(code, int(KW.LAST in ins.tags))
    US_CMN_INST.NOP(code, int(KW.NOP in ins.tags))
    US_CMN_INST.ALU_WAIT(code, int(KW.ALU_WAIT in ins.tags))

    emit_addr(code, ins.addr)
    if ins.alpha_op is not None:
        emit_alpha_op(code, ins.alpha_op)
    if ins.rgb_op is not None:
        emit_rgb_op(code, ins.rgb_op)
