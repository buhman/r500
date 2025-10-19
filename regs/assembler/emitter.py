from assembler.keywords import ME, VE, MVE, KW
from assembler.parser import Instruction, DestinationOp, Source
import pvs_dst
import pvs_src
import pvs_dst_bits
import pvs_src_bits

def we_x(s):
    return int(0 in s)

def we_y(s):
    return int(1 in s)

def we_z(s):
    return int(2 in s)

def we_w(s):
    return int(3 in s)

def dst_reg_type(kw):
    if kw == KW.temporary:
        return pvs_dst_bits.PVS_DST_REG_gen["TEMPORARY"]
    elif kw == KW.a0:
        return pvs_dst_bits.PVS_DST_REG_gen["A0"]
    elif kw == KW.out:
        return pvs_dst_bits.PVS_DST_REG_gen["OUT"]
    elif kw == KW.out_repl_x:
        return pvs_dst_bits.PVS_DST_REG_gen["OUT_REPL_X"]
    elif kw == KW.alt_temporary:
        return pvs_dst_bits.PVS_DST_REG_gen["ALT_TEMPORARY"]
    elif kw == KW.input:
        return pvs_dst_bits.PVS_DST_REG_gen["INPUT"]
    else:
        assert not "Invalid PVS_DST_REG", kw

def emit_destination_op(dst_op: DestinationOp):
    assert type(dst_op.opcode) in {ME, VE, MVE}
    math_inst = int(type(dst_op.opcode) is ME)
    if dst_op.macro:
        assert dst_op.opcode.value in {0, 1}
    ve_sat = int((not math_inst) and dst_op.sat)
    me_sat = int(math_inst and dst_op.sat)

    value = (
          pvs_dst.OPCODE_gen(dst_op.opcode.value)
        | pvs_dst.MATH_INST_gen(math_inst)
        | pvs_dst.MACRO_INST_gen(int(dst_op.macro))
        | pvs_dst.REG_TYPE_gen(dst_reg_type(dst_op.type))
        | pvs_dst.OFFSET_gen(dst_op.offset)
        | pvs_dst.WE_X_gen(we_x(dst_op.write_enable))
        | pvs_dst.WE_Y_gen(we_y(dst_op.write_enable))
        | pvs_dst.WE_Z_gen(we_z(dst_op.write_enable))
        | pvs_dst.WE_W_gen(we_w(dst_op.write_enable))
        | pvs_dst.VE_SAT_gen(ve_sat)
        | pvs_dst.ME_SAT_gen(me_sat)
    )
    yield value

def src_reg_type(kw):
    if kw == KW.temporary:
        return pvs_src_bits.PVS_SRC_REG_TYPE_gen["PVS_SRC_REG_TEMPORARY"]
    elif kw == KW.input:
        return pvs_src_bits.PVS_SRC_REG_TYPE_gen["PVS_SRC_REG_INPUT"]
    elif kw == KW.constant:
        return pvs_src_bits.PVS_SRC_REG_TYPE_gen["PVS_SRC_REG_CONSTANT"]
    elif kw == KW.alt_temporary:
        return pvs_src_bits.PVS_SRC_REG_TYPE_gen["PVS_SRC_REG_ALT_TEMPORARY"]
    else:
        assert not "Invalid PVS_SRC_REG", kw

def emit_source(src: Source, prev: Source):
    if src is not None:
        value = (
              pvs_src.REG_TYPE_gen(src_reg_type(src.type))
            | pvs_src.OFFSET_gen(src.offset)
            | pvs_src.SWIZZLE_X_gen(src.swizzle.select[0])
            | pvs_src.SWIZZLE_Y_gen(src.swizzle.select[1])
            | pvs_src.SWIZZLE_Z_gen(src.swizzle.select[2])
            | pvs_src.SWIZZLE_W_gen(src.swizzle.select[3])
            | pvs_src.MODIFIER_X_gen(int(src.swizzle.modifier[0]))
            | pvs_src.MODIFIER_Y_gen(int(src.swizzle.modifier[1]))
            | pvs_src.MODIFIER_Z_gen(int(src.swizzle.modifier[2]))
            | pvs_src.MODIFIER_W_gen(int(src.swizzle.modifier[3]))
        )
    else:
        assert prev is not None
        value = (
              pvs_src.REG_TYPE_gen(src_reg_type(prev.type))
            | pvs_src.OFFSET_gen(prev.offset)
            | pvs_src.SWIZZLE_X_gen(7)
            | pvs_src.SWIZZLE_Y_gen(7)
            | pvs_src.SWIZZLE_Z_gen(7)
            | pvs_src.SWIZZLE_W_gen(7)
            | pvs_src.MODIFIER_X_gen(0)
            | pvs_src.MODIFIER_Y_gen(0)
            | pvs_src.MODIFIER_Z_gen(0)
            | pvs_src.MODIFIER_W_gen(0)
        )
    yield value

def prev_source(ins, ix):
    if ix == 0:
        assert ins.source0 is not None
        return ins.source0
    elif ix == 1:
        return ins.source0
    elif ix == 2:
        if ins.source1 is not None:
            return ins.source1
        else:
            return ins.source0
    else:
        assert False, ix

def emit_instruction(ins: Instruction):
    yield from emit_destination_op(ins.destination_op)

    yield from emit_source(ins.source0, prev_source(ins, 0))
    yield from emit_source(ins.source1, prev_source(ins, 1))
    yield from emit_source(ins.source2, prev_source(ins, 2))
