from assembler.fs import alu_validator
from assembler.fs import tex_validator
from assembler.fs.parser import TEXInstruction, ALUInstruction

def validate_instruction(ins):
    if type(ins) is TEXInstruction:
        return tex_validator.validate_instruction(ins)
    elif type(ins) is ALUInstruction:
        return alu_validator.validate_instruction(ins)
    else:
        assert False, type(ins)
