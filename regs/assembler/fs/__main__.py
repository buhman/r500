import sys
import struct

from assembler.lexer import Lexer, LexerError
from assembler.parser import ParserError
from assembler.validator import ValidatorError
from assembler.fs.parser import Parser
from assembler.fs.keywords import find_keyword
from assembler.fs.validator import validate_instruction
from assembler.fs.emitter import emit_instruction
from assembler.error import print_error

def frontend_inner(filename, buf):
    lexer = Lexer(filename, buf, find_keyword, emit_newlines=False, minus_is_token=True)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    for ins_ast in parser.instructions():
        ins = validate_instruction(ins_ast)
        code = [0] * 6
        emit_instruction(code, ins)
        yield code

def frontend(filename, buf):
    try:
        yield from frontend_inner(filename, buf)
    except LexerError as e:
        print_error(e)
        raise
    except ParserError as e:
        print_error(e)
        raise
    except ValidatorError as e:
        print_error(e)
        raise

if __name__ == "__main__":
    assert len(sys.argv) in {2, 3}
    input_filename = sys.argv[1]
    binary = len(sys.argv) == 3
    if binary:
        output_filename = sys.argv[2]

    with open(input_filename, 'rb') as f:
        buf = f.read()

    code_gen = list(frontend(input_filename, buf))

    if not binary:
        for cw in code_gen:
            print("\n".join(f"0x{cw[i]:08x}," for i in range(6)))
            print()
    else:
        with open(output_filename, 'wb') as f:
            for cw in code_gen:
                data = struct.pack("<IIIIII", *cw)
                f.write(data)
