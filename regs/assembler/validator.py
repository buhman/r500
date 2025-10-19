from assembler.keywords import ME, VE, macro_vector_operations

class ValidatorError(Exception):
    pass

def validate_instruction(ins):
    addresses = len(set(
        source.offset
        for source in [ins.source0, ins.source1, ins.source2]
        if source is not None
    ))
    if addresses > 2:
        if type(ins.destination_op.opcode) is not VE:
            raise ValidatorError("too many addresses for non-VE instruction", ins)
        if ins.destination_op.opcode.name not in {b"VE_MULTIPLYX2_ADD", b"VE_MULTIPLY_ADD"}:
            raise ValidatorError("too many addresses for VE non-multiply-add instruction", ins)
        assert ins.destination_op.macro == False, ins
        ins.destination_op.macro = True
        if ins.destination_op.opcode.name == b"VE_MULTIPLY_ADD":
            ins.destination_op.opcode = macro_vector_operations[0]
        elif ins.destination_op.opcode.name == b"VE_MULTIPLYX2_ADD":
            ins.destination_op.opcode = macro_vector_operations[1]
        else:
            assert False
    return ins
