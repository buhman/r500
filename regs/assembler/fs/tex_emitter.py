from assembler.fs.common_validator import InstructionType
from assembler.fs.keywords import KW

from assembler.fs.common_emitter import US_CMN_INST
from assembler.fs.common_emitter import US_TEX_INST
from assembler.fs.common_emitter import US_TEX_ADDR
from assembler.fs.common_emitter import US_TEX_ADDR_DXDY

def emit_instruction(code, ins):
    US_CMN_INST.TYPE(code, InstructionType.TEX)
    US_CMN_INST.TEX_SEM_WAIT(code, int(KW.TEX_SEM_WAIT in ins.tags))
    US_CMN_INST.LAST(code, int(KW.LAST in ins.tags))
    US_CMN_INST.NOP(code, int(KW.NOP in ins.tags))
    US_CMN_INST.ALU_WAIT(code, int(KW.ALU_WAIT in ins.tags))
    US_CMN_INST.RGB_WMASK(code, ins.masks.rgb_wmask.value)
    US_CMN_INST.ALPHA_WMASK(code, ins.masks.alpha_wmask.value)
    US_CMN_INST.RGB_OMASK(code, ins.masks.rgb_omask.value)
    US_CMN_INST.ALPHA_OMASK(code, ins.masks.alpha_omask.value)

    US_TEX_INST.TEX_ID(code, ins.tex_id)
    US_TEX_INST.INST(code, ins.opcode.value)
    US_TEX_INST.TEX_SEM_ACQUIRE(code, int(KW.TEX_SEM_ACQUIRE in ins.tags))
    US_TEX_INST.IGNORE_UNCOVERED(code, 0)
    US_TEX_INST.UNSCALED(code, 0)

    US_TEX_ADDR.SRC_ADDR(code, ins.src_addr)
    US_TEX_ADDR.SRC_S_SWIZ(code, ins.src_swizzle[0].value)
    US_TEX_ADDR.SRC_T_SWIZ(code, ins.src_swizzle[1].value)
    US_TEX_ADDR.SRC_R_SWIZ(code, ins.src_swizzle[2].value)
    US_TEX_ADDR.SRC_Q_SWIZ(code, ins.src_swizzle[3].value)

    US_TEX_ADDR.DST_ADDR(code, ins.dst_addr)
    US_TEX_ADDR.DST_R_SWIZ(code, ins.dst_swizzle[0].value)
    US_TEX_ADDR.DST_G_SWIZ(code, ins.dst_swizzle[1].value)
    US_TEX_ADDR.DST_B_SWIZ(code, ins.dst_swizzle[2].value)
    US_TEX_ADDR.DST_A_SWIZ(code, ins.dst_swizzle[3].value)
