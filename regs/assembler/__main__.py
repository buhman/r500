from assembler.lexer import Lexer
from assembler.parser import Parser
from assembler.emitter import emit_instruction

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

if __name__ == "__main__":

    #buf = b"out[0].xz    = VE_MAD    input[0].-y-_-0-_ temp[0].x_0_      temp[0].y_0_"
    buf = sample
    lexer = Lexer(buf)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    for ins in parser.instructions():
        print("\n".join(
            f"{value:08x}"
            for value in emit_instruction(ins)
        ))
