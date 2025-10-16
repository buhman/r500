import sys

from assembler.lexer import Lexer, LexerError
from assembler.parser import Parser, ParserError
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

def frontend_inner(buf):
    lexer = Lexer(buf)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    for ins, start_end in parser.instructions():
        yield list(emit_instruction(ins)), start_end

def print_error(filename, buf, e):
    assert len(e.args) == 2, e
    message, token = e.args
    lines = buf.splitlines()
    line = lines[token.line - 1]

    error_name = str(type(e).__name__)
    col_indent = ' ' * token.col
    col_pointer = '^' * len(token.lexeme)
    RED = "\033[0;31m"
    DEFAULT = "\033[0;0m"
    print(f'File: "{filename}", line {token.line}, column {token.col}\n', file=sys.stderr)
    sys.stderr.write('    ')
    wrote_default = False
    for i, c in enumerate(line.decode('utf-8')):
        if i == token.col:
            sys.stderr.write(RED)
        sys.stderr.write(c)
        if i == token.col + len(token.lexeme):
            wrote_default = True
            sys.stderr.write(DEFAULT)
    if not wrote_default:
        sys.stderr.write(DEFAULT)
    sys.stderr.write('\n')
    print(f"    {RED}{col_indent}{col_pointer}{DEFAULT}", file=sys.stderr)
    print(f'{RED}{error_name}{DEFAULT}: {message}', file=sys.stderr)

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
    output = list(frontend(input_filename, buf))
    for cw, (start_ix, end_ix) in output:
        if True:
            print(f"0x{cw[0]:08x}, 0x{cw[1]:08x}, 0x{cw[2]:08x}, 0x{cw[3]:08x},")
        else:
            source = buf[start_ix:end_ix]
            print(f"0x{cw[0]:08x}, 0x{cw[1]:08x}, 0x{cw[2]:08x}, 0x{cw[3]:08x},  // {source.decode('utf-8')}")
