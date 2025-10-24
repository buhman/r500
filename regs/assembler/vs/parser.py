from itertools import pairwise
from dataclasses import dataclass
from typing import Union

from assembler.parser import BaseParser, ParserError
from assembler.lexer import TT, Token
from assembler.error import print_error

@dataclass
class Destination:
    type_keyword: Token
    offset_identifier: Token
    write_enable_identifier: Token

@dataclass
class Source:
    absolute: bool
    type_keyword: Token
    offset_identifier: Token
    swizzle_identifier: Token

@dataclass
class Operation:
    destination: Destination
    opcode_keyword: Token
    opcode_suffix_keyword: Token
    sources: list[Source]

@dataclass
class Instruction:
    operations: list[Operation]

class Parser(BaseParser):
    def destination(self):
        type_keyword = self.consume(TT.keyword, "expected destination type keyword")
        self.consume(TT.left_square, "expected left square")
        offset_identifier = self.consume(TT.identifier, "expected destination offset identifier")
        self.consume(TT.right_square, "expected right square")
        self.consume(TT.dot, "expected dot")
        write_enable_identifier = self.consume(TT.identifier, "expected destination write enable identifier")

        return Destination(
            type_keyword,
            offset_identifier,
            write_enable_identifier,
        )

    def is_absolute(self):
        result = self.match(TT.bar)
        if result:
            self.advance()
        return result

    def source(self):
        absolute = self.is_absolute()

        type_keyword = self.consume(TT.keyword, "expected source type keyword")
        self.consume(TT.left_square, "expected left square")
        offset_identifier = self.consume(TT.identifier, "expected source offset identifier")
        self.consume(TT.right_square, "expected right square")
        self.consume(TT.dot, "expected dot")
        swizzle_identifier = self.consume(TT.identifier, "expected source swizzle identifier")

        if absolute:
            self.consume(TT.bar, "expected vertical bar")

        return Source(
            absolute,
            type_keyword,
            offset_identifier,
            swizzle_identifier,
        )

    def operation(self):
        destination = self.destination()

        self.consume(TT.equal, "expected equal")

        opcode_keyword = self.consume(TT.keyword, "expected opcode keyword")
        opcode_suffix_keyword = None
        if self.match(TT.dot):
            self.advance()
            opcode_suffix_keyword = self.consume(TT.keyword, "expected opcode suffix keyword")

        sources = []
        while not (self.match(TT.comma) or self.match(TT.semicolon)):
            sources.append(self.source())

        return Operation(
            destination,
            opcode_keyword,
            opcode_suffix_keyword,
            sources,
        )

    def instruction(self):
        operations = []
        while not self.match(TT.semicolon):
            operations.append(self.operation())
            if not self.match(TT.semicolon):
                self.consume(TT.comma, "expected comma")

        self.consume(TT.semicolon, "expected semicolon")
        return Instruction(
            operations,
        )

    def instructions(self):
        while not self.match(TT.eof):
            yield self.instruction()

if __name__ == "__main__":
    from assembler.lexer import Lexer
    from assembler.vs.keywords import find_keyword
    buf = b"""
  out[0].xz    = VE_MAD    input[0].-y-_-0-_ temp[0].x_0_ ,
atemp[0].xz    = ME_SIN    input[0].-y-_-0-_ ;
"""
    lexer = Lexer(buf, find_keyword, emit_newlines=False, minus_is_token=False)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    from pprint import pprint
    try:
        pprint(parser.instruction())
    except ParserError as e:
        print_error(None, buf, e)
        raise
    print(parser.peek())
