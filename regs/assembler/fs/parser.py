from enum import IntEnum
from typing import Literal, Union
from dataclasses import dataclass

from assembler.parser import BaseParser, ParserError
from assembler.lexer import TT, Token
from assembler.fs.keywords import KW, find_keyword
from assembler.error import print_error

class Mod(IntEnum):
    nop = 0
    neg = 1
    abs = 2
    nab = 3

@dataclass
class LetExpression:
    src_keyword: Token
    src_swizzle_identifier: Token
    addr_keyword: Token
    addr_value_identifier: Token

@dataclass
class DestAddrSwizzle:
    dest_keyword: Token
    addr_identifier: Token
    swizzle_identifier: Token

@dataclass
class SwizzleSel:
    sel_keyword: Token
    swizzle_identifier: Token
    mod: Mod

@dataclass
class Operation:
    dest_addr_swizzles: list[DestAddrSwizzle]
    opcode_keyword: Token
    swizzle_sels: list[SwizzleSel]

@dataclass
class Instruction:
    out: bool
    tex_sem_wait: bool
    let_expressions: list[LetExpression]
    operations: list[Operation]

class Parser(BaseParser):
    def let_expression(self):
        src_keyword = self.consume(TT.keyword, "expected src keyword")
        self.consume(TT.dot, "expected dot")
        src_swizzle_identifier = self.consume(TT.identifier, "expected src swizzle identifier")
        self.consume(TT.equal, "expected equal")
        addr_keyword = self.consume(TT.keyword, "expected addr keyword")

        if addr_keyword.keyword in {KW.NEG2, KW.SUB, KW.ADD, KW.NEG}:
            addr_value_identifier = None
        else:
            if addr_keyword.keyword is KW.FLOAT:
                self.consume(TT.left_paren, "expected left paren")
            else:
                self.consume(TT.left_square, "expected left square")

            addr_value_identifier = self.consume(TT.identifier, "expected address identifier")

            if addr_keyword.keyword is KW.FLOAT:
                self.consume(TT.right_paren, "expected right paren")
            else:
                self.consume(TT.right_square, "expected right square")

        return LetExpression(
            src_keyword,
            src_swizzle_identifier,
            addr_keyword,
            addr_value_identifier,
        )

    def dest_addr_swizzle(self):
        dest_keyword = self.consume(TT.keyword, "expected dest keyword")
        self.consume(TT.left_square, "expected left square")
        addr_identifier = self.consume(TT.identifier, "expected dest addr identifier")
        self.consume(TT.right_square, "expected left square")
        self.consume(TT.dot, "expected dot")
        swizzle_identifier = self.consume(TT.identifier, "expected dest swizzle identifier")
        self.consume(TT.equal, "expected equal")
        return DestAddrSwizzle(
            dest_keyword,
            addr_identifier,
            swizzle_identifier,
        )

    def is_opcode(self):
        opcode_keywords = {
            KW.CMP, KW.CND, KW.COS, KW.D2A,
            KW.DP , KW.DP3, KW.DP4, KW.EX2,
            KW.FRC, KW.LN2, KW.MAD, KW.MAX,
            KW.MDH, KW.MDV, KW.MIN, KW.RCP,
            KW.RSQ, KW.SIN, KW.SOP,
        }
        if self.match(TT.keyword):
            token = self.peek()
            return token.keyword in opcode_keywords
        return False

    def is_neg(self):
        result = self.match(TT.minus)
        if result:
            self.advance()
        return result

    def is_abs(self):
        result = self.match(TT.bar)
        if result:
            self.advance()
        return result

    def swizzle_sel(self):
        neg = self.is_neg()
        abs = self.is_abs()

        sel_keyword = self.consume(TT.keyword, "expected sel keyword")
        self.consume(TT.dot, "expected dot")
        swizzle_identifier = self.consume(TT.identifier, "expected swizzle identifier")

        if abs:
            self.consume(TT.bar, "expected bar")

        mod_table = {
            # (neg, abs)
            (False, False): Mod.nop,
            (False, True): Mod.abs,
            (True, False): Mod.neg,
            (True, True): Mod.nab,
        }
        mod = mod_table[(neg, abs)]
        return SwizzleSel(
            sel_keyword,
            swizzle_identifier,
            mod,
        )

    def operation(self):
        dest_addr_swizzles = []
        while not self.is_opcode():
            dest_addr_swizzles.append(self.dest_addr_swizzle())

        opcode_keyword = self.consume(TT.keyword, "expected opcode keyword")

        swizzle_sels = []
        while not (self.match(TT.comma) or self.match(TT.semicolon)):
            swizzle_sels.append(self.swizzle_sel())

        return Operation(
            dest_addr_swizzles,
            opcode_keyword,
            swizzle_sels
        )

    def instruction(self):
        out = False
        if self.match_keyword(KW.OUT):
            self.advance()
            out = True
        tex_sem_wait = False
        if self.match_keyword(KW.TEX_SEM_WAIT):
            self.advance()
            tex_sem_wait = True

        let_expressions = []
        while not self.match(TT.colon):
            let_expressions.append(self.let_expression())
            if not self.match(TT.colon):
                self.consume(TT.comma, "expected comma")

        self.consume(TT.colon, "expected colon")

        operations = []
        while not self.match(TT.semicolon):
            operations.append(self.operation())
            if not self.match(TT.semicolon):
                self.consume(TT.comma, "expected comma")

        self.consume(TT.semicolon, "expected semicolon")

        return Instruction(
            out,
            tex_sem_wait,
            let_expressions,
            operations,
        )

    def instructions(self):
        while not self.match(TT.eof):
            yield self.instruction()

if __name__ == "__main__":
    from assembler.lexer import Lexer
    buf = b"""
src0.a = float(0), src0.rgb = temp[0] :
  out[0].none = temp[0].none = MAD src0.r src0.r src0.r ,
  out[0].none = temp[0].r    = DP3 src0.rg0 src0.rg0 ;
    """
    buf = b"""
OUT TEX_SEM_WAIT
src0.a = float(0), src1.a = float(0), src2.a = float(0), srcp.a = neg2, src0.rgb = temp[0], src1.rgb = float(0), src2.rgb = float(0), srcp.rgb = neg2 :
  out[0].none = temp[0].none = MAD src0.r src0.r src0.r ,
  out[0].none = temp[0].r    = DP3 src0.rg0 src0.rg0 src0.rrr ;
"""
    lexer = Lexer(buf, find_keyword, emit_newlines=False, minus_is_token=True)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    from pprint import pprint
    try:
        pprint(parser.instruction())
    except ParserError as e:
        print_error(None, buf, e)
        raise
    print(parser.peek())
