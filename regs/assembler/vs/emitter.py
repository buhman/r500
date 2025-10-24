from typing import Union

from assembler.vs.opcodes import ME, VE, MVE
from assembler.vs.validator import Destination, Source, Instruction
from assembler.vs.validator import DualMathVEOperation, DualMathMEOperation, DualMathInstruction

import pvs_dst
import pvs_src
import pvs_dual_math

def emit_destination_opcode(destination: Destination,
                            opcode: Union[ME, VE, MVE],
                            saturation: bool):
    assert type(opcode) in {ME, VE, MVE}
    math_inst = int(type(opcode) is ME)
    macro_inst = int(type(opcode) is MVE)

    ve_sat = int((not math_inst) and saturation)
    me_sat = int(math_inst and saturation)

    value = (
          pvs_dst.OPCODE_gen(opcode.value)
        | pvs_dst.MATH_INST_gen(math_inst)
        | pvs_dst.MACRO_INST_gen(int(macro_inst))
        | pvs_dst.REG_TYPE_gen(destination.type.value)
        | pvs_dst.OFFSET_gen(destination.offset)
        | pvs_dst.WE_X_gen(int(destination.write_enable[0]))
        | pvs_dst.WE_Y_gen(int(destination.write_enable[1]))
        | pvs_dst.WE_Z_gen(int(destination.write_enable[2]))
        | pvs_dst.WE_W_gen(int(destination.write_enable[3]))
        | pvs_dst.VE_SAT_gen(ve_sat)
        | pvs_dst.ME_SAT_gen(me_sat)
        | pvs_dst.DUAL_MATH_OP_gen(0)
    )
    yield value

def emit_source(src: Source, prev: Source):
    if src is not None:
        assert src.offset <= 255
        value = (
              pvs_src.REG_TYPE_gen(src.type.value)
            | pvs_src.OFFSET_gen(src.offset)
            | pvs_src.ABS_XYZW_gen(int(src.absolute))
            | pvs_src.SWIZZLE_X_gen(src.swizzle_selects[0].value)
            | pvs_src.SWIZZLE_Y_gen(src.swizzle_selects[1].value)
            | pvs_src.SWIZZLE_Z_gen(src.swizzle_selects[2].value)
            | pvs_src.SWIZZLE_W_gen(src.swizzle_selects[3].value)
            | pvs_src.MODIFIER_X_gen(int(src.modifiers[0]))
            | pvs_src.MODIFIER_Y_gen(int(src.modifiers[1]))
            | pvs_src.MODIFIER_Z_gen(int(src.modifiers[2]))
            | pvs_src.MODIFIER_W_gen(int(src.modifiers[3]))
        )
    else:
        assert prev is not None
        assert prev.offset <= 255
        value = (
              pvs_src.REG_TYPE_gen(prev.type.value)
            | pvs_src.OFFSET_gen(prev.offset)
            | pvs_src.ABS_XYZW_gen(0)
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
    assert ins.sources[0] is not None
    if ix == 0:
        return ins.sources[0]
    elif ix == 1:
        return ins.sources[0]
    elif ix == 2:
        if len(ins.sources) >= 2 and ins.sources[1] is not None:
            return ins.sources[1]
        else:
            return ins.sources[0]
    else:
        assert False, ix

def emit_instruction(ins: Instruction):
    yield from emit_destination_opcode(ins.destination,
                                       ins.opcode,
                                       ins.saturation)

    if len(ins.sources) >= 1:
        source0 = ins.sources[0]
        source1 = ins.sources[1] if len(ins.sources) >= 2 else None
        source2 = ins.sources[2] if len(ins.sources) >= 3 else None
        yield from emit_source(source0, prev_source(ins, 0))
        yield from emit_source(source1, prev_source(ins, 1))
        yield from emit_source(source2, prev_source(ins, 2))
    else:
        yield 0
        yield 0
        yield 0


def emit_dual_math_destination_opcode(ve_operation: DualMathVEOperation,
                                      me_operation: DualMathMEOperation):
    assert type(ve_operation.opcode) is VE
    assert type(me_operation.opcode) is ME

    ve_sat = int(ve_operation.saturation)
    me_sat = int(me_operation.saturation)

    destination = ve_operation.destination
    value = (
          pvs_dst.OPCODE_gen(ve_operation.opcode.value)
        | pvs_dst.MATH_INST_gen(0)
        | pvs_dst.MACRO_INST_gen(0)
        | pvs_dst.REG_TYPE_gen(destination.type.value)
        | pvs_dst.OFFSET_gen(destination.offset)
        | pvs_dst.WE_X_gen(int(destination.write_enable[0]))
        | pvs_dst.WE_Y_gen(int(destination.write_enable[1]))
        | pvs_dst.WE_Z_gen(int(destination.write_enable[2]))
        | pvs_dst.WE_W_gen(int(destination.write_enable[3]))
        | pvs_dst.VE_SAT_gen(ve_sat)
        | pvs_dst.ME_SAT_gen(me_sat)
        | pvs_dst.DUAL_MATH_OP_gen(1)
    )
    yield value

def emit_dual_math_dual_math_instruction(operation: DualMathMEOperation):
    opcode_upper = (operation.opcode.value >> 4) & 0b1
    opcode_lower = (operation.opcode.value >> 0) & 0b1111

    if len(operation.sources) == 1:
        source, = operation.sources
        value = (
              pvs_dual_math.SRC_REG_TYPE_gen(source.type.value)
            | pvs_dual_math.DST_OPCODE_MSB_gen(opcode_upper)
            | pvs_dual_math.SRC_ABS_XYZW_gen(int(source.absolute))
            | pvs_dual_math.SRC_OFFSET_gen(source.offset)
            | pvs_dual_math.SRC_SWIZZLE_X_gen(source.swizzle_selects[0].value)
            | pvs_dual_math.SRC_SWIZZLE_Y_gen(source.swizzle_selects[1].value)
            | pvs_dual_math.DUAL_MATH_DST_OFFSET_gen(operation.destination.offset)
            | pvs_dual_math.DST_OPCODE_gen(opcode_lower)
            | pvs_dual_math.SRC_MODIFIER_X_gen(int(source.modifiers[0]))
            | pvs_dual_math.SRC_MODIFIER_Y_gen(int(source.modifiers[1]))
            | pvs_dual_math.DST_WE_SEL_gen(operation.destination.write_enable.value)
        )
    else:
        value = (
              pvs_dual_math.DST_OPCODE_MSB_gen(opcode_upper)
            | pvs_dual_math.DUAL_MATH_DST_OFFSET_gen(operation.destination.offset)
            | pvs_dual_math.DST_OPCODE_gen(opcode_lower)
            | pvs_dual_math.DST_WE_SEL_gen(operation.destination.write_enable.value)
        )
    yield value

def emit_dual_math_instruction(ins: DualMathInstruction):
    yield from emit_dual_math_destination_opcode(ins.ve_operation, ins.me_operation)
    if len(ins.ve_operation.sources) >= 1:
        source0 = ins.ve_operation.sources[0]
        source1 = ins.ve_operation.sources[1] if len(ins.ve_operation.sources) >= 2 else None
        yield from emit_source(source0, prev_source(ins.ve_operation, 0))
        yield from emit_source(source1, prev_source(ins.ve_operation, 1))
    else:
        yield 0
        yield 0
    yield from emit_dual_math_dual_math_instruction(ins.me_operation)
