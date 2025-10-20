import sys

from assembler.lexer import Lexer, LexerError
from assembler.fs.parser import Parser, ParserError
from assembler.fs.keywords import find_keyword
from assembler.fs.validator import validate_instruction, ValidatorError
from assembler.fs.emitter import emit_instruction
from assembler.error import print_error

def frontend_inner(buf):
    lexer = Lexer(buf, find_keyword, emit_newlines=False)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    for ins_ast in parser.instructions():
        ins = validate_instruction(ins_ast)
        code = [0] * 6
        emit_instruction(code, ins)
        print("\n".join(f"0x{code[i]:08x}," for i in range(6)))
        print()

def frontend(filename, buf):
    try:
        frontend_inner(buf)
    except LexerError as e:
        print_error(filename, buf, e)
        raise
    except ParserError as e:
        print_error(filename, buf, e)
        raise
    except ValidatorError as e:
        print_error(filename, buf, e)
        raise

if __name__ == "__main__":
    input_filename = sys.argv[1]
    with open(input_filename, 'rb') as f:
        buf = f.read()
    frontend(input_filename, buf)
