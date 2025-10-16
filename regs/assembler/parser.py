from itertools import pairwise
from dataclasses import dataclass
from typing import Union

from assembler import lexer
from assembler.lexer import TT, Token
from assembler.keywords import KW, ME, VE

"""
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

@dataclass
class DestinationOp:
    type: KW
    offset: int
    write_enable: set[int]
    opcode: Union[VE, ME]

@dataclass
class SourceSwizzle:
    select: tuple[int, int, int, int]
    modifier: tuple[bool, bool, bool, bool]

@dataclass
class Source:
    type: KW
    offset: int
    swizzle: SourceSwizzle

@dataclass
class Instruction:
    destination_op: DestinationOp
    source0: Source
    source1: Source
    source2: Source

class ParserError(Exception):
    pass

def identifier_to_number(token):
    digits = set(b"0123456789")

    assert token.type is TT.identifier
    if not all(d in digits for d in token.lexeme):
        raise ParserError("expected number", token)
    return int(bytes(token.lexeme), 10)

def we_ord(c):
    if c == ord("w"):
        return 3
    else:
        return c - ord("x")

def parse_dest_write_enable(token):
    we_chars = set(b"xyzw")
    assert token.type is TT.identifier
    we = bytes(token.lexeme).lower()
    if not all(c in we_chars for c in we):
        raise ParserError("expected destination write enable", token)
    if not all(we_ord(a) < we_ord(b) for a, b in pairwise(we)) or len(set(we)) != len(we):
        raise ParserError("misleading non-sequential write enable", token)
    return set(we_ord(c) for c in we)

def parse_source_swizzle(token):
    select_mapping = {
        ord('x'): 0,
        ord('y'): 1,
        ord('z'): 2,
        ord('w'): 3,
        ord('0'): 4,
        ord('1'): 5,
        ord('h'): 6,
        ord('_'): 7,
        ord('u'): 7,
    }
    state = 0
    ix = 0
    swizzle_selects = [None] * 4
    swizzle_modifiers = [None] * 4
    lexeme = bytes(token.lexeme).lower()
    while state < 4:
        if ix >= len(token.lexeme):
            raise ParserError("invalid source swizzle", token)
        c = lexeme[ix]
        if c == ord('-'):
            if (swizzle_modifiers[state] is not None) or (swizzle_selects[state] is not None):
                raise ParserError("invalid source swizzle modifier", token)
            swizzle_modifiers[state] = True
        elif c in select_mapping:
            if swizzle_selects[state] is not None:
                raise ParserError("invalid source swizzle select", token)
            swizzle_selects[state] = select_mapping[c]
            if swizzle_modifiers[state] is None:
                swizzle_modifiers[state] = False
            state += 1
        else:
            raise ParserError("invalid source swizzle", token)
        ix += 1
    if ix != len(lexeme):
        raise ParserError("invalid source swizzle", token)
    return SourceSwizzle(swizzle_selects, swizzle_modifiers)

class Parser:
    def __init__(self, tokens: list[lexer.Token]):
        self.current_ix = 0
        self.tokens = tokens

    def peek(self, offset=0):
        token = self.tokens[self.current_ix + offset]
        #print(token)
        return token

    def at_end_p(self):
        return self.peek().type == TT.eof

    def advance(self):
        token = self.peek()
        self.current_ix += 1
        return token

    def match(self, token_type):
        token = self.peek()
        return token.type == token_type

    def consume(self, token_type, message):
        token = self.advance()
        if token.type != token_type:
            raise ParserError(message, token)
        return token

    def consume_either(self, token_type1, token_type2, message):
        token = self.advance()
        if token.type != token_type1 and token.type != token_type2:
            raise ParserError(message, token)
        return token

    def destination_type(self):
        token = self.consume(TT.keyword, "expected destination type")
        destination_keywords = {KW.temporary, KW.a0, KW.out, KW.out_repl_x, KW.alt_temporary, KW.input}
        if token.keyword not in destination_keywords:
            raise ParserError("expected destination type", token)
        return token.keyword

    def offset(self):
        self.consume(TT.left_square, "expected offset")
        identifier_token = self.consume(TT.identifier, "expected offset")
        value = identifier_to_number(identifier_token)
        self.consume(TT.right_square, "expected offset")
        return value

    def opcode(self):
        token = self.consume(TT.keyword, "expected opcode")
        if type(token.keyword) != VE and type(token.keyword) != ME:
            raise ParserError("expected opcode", token)
        return token.keyword

    def destination_op(self):
        destination_type = self.destination_type()
        offset_value = self.offset()
        self.consume(TT.dot, "expected write enable")
        write_enable_token = self.consume(TT.identifier, "expected write enable token")
        write_enable = parse_dest_write_enable(write_enable_token)
        self.consume(TT.equal, "expected equals")
        opcode = self.opcode()
        return DestinationOp(destination_type, offset_value, write_enable, opcode)

    def source_type(self):
        token = self.consume(TT.keyword, "expected source type")
        source_keywords = {KW.temporary, KW.input, KW.constant, KW.alt_temporary}
        if token.keyword not in source_keywords:
            raise ParserError("expected source type", token)
        return token.keyword

    def source_swizzle(self):
        token = self.consume(TT.identifier, "expected source swizzle")
        return parse_source_swizzle(token)

    def source(self):
        "input[0].-y-_-0-_"
        source_type = self.source_type()
        offset = self.offset()
        self.consume(TT.dot, "expected source swizzle")
        source_swizzle = self.source_swizzle()
        return Source(source_type, offset, source_swizzle)

    def instruction(self):
        while self.match(TT.eol):
            self.advance()
        first_token = self.peek()
        destination_op = self.destination_op()
        source0 = self.source()
        if self.match(TT.eol) or self.match(TT.eof):
            source1 = None
        else:
            source1 = self.source()
        if self.match(TT.eol) or self.match(TT.eof):
            source2 = None
        else:
            source2 = self.source()
        last_token = self.peek(-1)
        self.consume_either(TT.eol, TT.eof, "expected newline or EOF")
        return (
            Instruction(destination_op, source0, source1, source2),
            (first_token.start_ix, last_token.start_ix + len(last_token.lexeme))
        )

    def instructions(self):
        while not self.match(TT.eof):
            yield self.instruction()

if __name__ == "__main__":
    from assembler.lexer import Lexer
    buf = b"out[0].xz    = VE_MAD    input[0].-y-_-0-_ temp[0].x_0_      temp[0].y_0_"
    lexer = Lexer(buf)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    from pprint import pprint
    pprint(parser.instruction())
