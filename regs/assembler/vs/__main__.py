import sys

from assembler.lexer import Lexer, LexerError
from assembler.vs.keywords import find_keyword
from assembler.vs.parser import Parser, ParserError
from assembler.vs.emitter import emit_instruction, emit_dual_math_instruction
from assembler.vs.validator import validate_instruction, Instruction, DualMathInstruction
from assembler.error import print_error

sample = b"""
temp[0].xyzw = VE_ADD    const[1].xyzw     const[1].0000     const[1].0000
temp[1].xyzw = VE_ADD    const[1].xyzw     const[1].0000     const[1].0000
temp[0].x    = VE_MAD    const[0].x___     temp[1].x___      temp[0].y___
temp[0].x    = VE_FRC    temp[0].x___      temp[0].0000      temp[0].0000
temp[0].x    = VE_MAD    temp[0].x___      const[1].z___     const[1].w___
temp[0].y    = ME_COS    temp[0].xxxx      temp[0].0000      temp[0].0000
temp[0].x    = ME_SIN    temp[0].xxxx      temp[0].0000      temp[0].0000
temp[0].yz   = VE_MUL    input[0]._xy_     temp[0]._yy_      temp[0].0000
out[0].xz    = VE_MAD    input[0].-y-_-0-_ temp[0].x_0_      temp[0].y_0_
out[0].yw    = VE_MAD    input[0]._x_0     temp[0]._x_0      temp[0]._z_1
"""

def frontend_inner(buf):
    lexer = Lexer(buf, find_keyword)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    for ins in parser.instructions():
        ins = validate_instruction(ins)
        if type(ins) is Instruction:
            yield list(emit_instruction(ins))
        elif type(ins) is DualMathInstruction:
            yield list(emit_dual_math_instruction(ins))
        else:
            assert False, type(ins)

def frontend(filename, buf):
    try:
        yield from frontend_inner(buf)
    except ParserError as e:
        print_error(input_filename, buf, e)
        raise
    except LexerError as e:
        print_error(input_filename, buf, e)
        raise

if __name__ == "__main__":
    input_filename = sys.argv[1]
    #output_filename = sys.argv[2]
    with open(input_filename, 'rb') as f:
        buf = f.read()
    for cw in frontend(input_filename, buf):
        print(f"0x{cw[0]:08x}, 0x{cw[1]:08x}, 0x{cw[2]:08x}, 0x{cw[3]:08x},")
