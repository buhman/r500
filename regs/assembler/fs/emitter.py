from assembler.fs import alu_emitter
from assembler.fs import tex_emitter
from assembler.fs import alu_validator
from assembler.fs import tex_validator

def emit_instruction(code, ins):
    if type(ins) is alu_validator.Instruction:
        return alu_emitter.emit_instruction(code, ins)
    elif type(ins) is tex_validator.Instruction:
        return tex_emitter.emit_instruction(code, ins)
    else:
        assert False, type(ins)
