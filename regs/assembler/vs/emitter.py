from typing import Union

from assembler.vs.opcodes import ME, VE, MVE
from assembler.vs.validator import Destination, Source, Instruction

import pvs_dst
import pvs_src

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
        if ins.sources[1] is not None:
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
        yield from emit_source(ins.sources[0], prev_source(ins, 0))

        source1 = ins.sources[1] if len(ins.sources) >= 2 else None
        source2 = ins.sources[2] if len(ins.sources) >= 3 else None
        yield from emit_source(source1, prev_source(ins, 1))
        yield from emit_source(source2, prev_source(ins, 2))
    else:
        yield 0
        yield 0
        yield 0
