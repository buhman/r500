from enum import IntEnum
from typing import Literal, Union
from dataclasses import dataclass

from assembler.parser import BaseParser, ParserError
from assembler.lexer import TT, Token
from assembler.fs.keywords import KW, find_keyword
from assembler.error import print_error

class ALUMod(IntEnum):
    nop = 0
    neg = 1
    abs = 2
    nab = 3

@dataclass
class ALULetExpression:
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
class ALUSwizzleSel:
    sel_keyword: Token
    swizzle_identifier: Token
    mod: ALUMod

@dataclass
class ALUOperation:
    dest_addr_swizzles: list[DestAddrSwizzle]
    omod: tuple[Token, Token]
    opcode_keyword: Token
    swizzle_sels: list[ALUSwizzleSel]

@dataclass
class ALUInstruction:
    tags: set[Token]
    let_expressions: list[ALULetExpression]
    operations: list[ALUOperation]

@dataclass
class TEXOperation:
    dest_addr_swizzles: list[DestAddrSwizzle]
    opcode_keyword: Token
    tex_id_identifier: Token
    tex_dst_swizzle_identifier: Token
    tex_src_address_identifier: Token
    tex_src_swizzle_identifier: Token

@dataclass
class TEXInstruction:
    tags: set[Token]
    operation: TEXOperation

class Parser(BaseParser):
    def alu_let_expression(self):
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

        return ALULetExpression(
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

    def alu_is_opcode(self):
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

    def alu_is_omod(self):
        is_omod = (
            self.match(TT.identifier, offset=0)
            and self.match(TT.dot, offset=1)
            and self.match(TT.identifier, offset=2)
            and self.match(TT.star, offset=3)
        )
        return is_omod

    def alu_is_neg(self):
        result = self.match(TT.minus)
        if result:
            self.advance()
        return result

    def alu_is_abs(self):
        result = self.match(TT.bar)
        if result:
            self.advance()
        return result

    def alu_swizzle_sel(self):
        neg = self.alu_is_neg()
        abs = self.alu_is_abs()

        sel_keyword = self.consume(TT.keyword, "expected sel keyword")
        self.consume(TT.dot, "expected dot")
        swizzle_identifier = self.consume(TT.identifier, "expected swizzle identifier")

        if abs:
            self.consume(TT.bar, "expected vertical bar")

        mod_table = {
            # (neg, abs)
            (False, False): ALUMod.nop,
            (False, True): ALUMod.abs,
            (True, False): ALUMod.neg,
            (True, True): ALUMod.nab,
        }
        mod = mod_table[(neg, abs)]
        return ALUSwizzleSel(
            sel_keyword,
            swizzle_identifier,
            mod,
        )

    def alu_operation(self):
        dest_addr_swizzles = []
        while not (self.alu_is_opcode() or self.alu_is_omod()):
            dest_addr_swizzles.append(self.dest_addr_swizzle())

        omod = None
        if self.alu_is_omod():
            omod_integer = self.consume(TT.identifier, "expected omod decimal identifier")
            self.consume(TT.dot, "expected omod decimal dot")
            omod_decimal = self.consume(TT.identifier, "expected omod decimal identifier")
            self.consume(TT.star, "expected omod star")
            omod = (omod_integer, omod_decimal)

        opcode_keyword = self.consume(TT.keyword, "expected opcode keyword")

        swizzle_sels = []
        while not (self.match(TT.comma) or self.match(TT.semicolon)):
            swizzle_sels.append(self.alu_swizzle_sel())

        return ALUOperation(
            dest_addr_swizzles,
            omod,
            opcode_keyword,
            swizzle_sels
        )

    def alu_instruction(self, out: bool):
        tags = list()
        tag_keywords = set([KW.OUT, KW.TEX_SEM_WAIT, KW.NOP, KW.LAST, KW.ALU_WAIT])
        while True:
            if self.match(TT.keyword):
                token = self.peek()
                if token.keyword in tag_keywords:
                    self.advance()
                    tag_keywords.remove(token.keyword)
                    tags.append(token)
                    continue
            break

        let_expressions = []
        while not self.match(TT.colon):
            let_expressions.append(self.alu_let_expression())
            if not self.match(TT.colon):
                self.consume(TT.comma, "expected comma")

        self.consume(TT.colon, "expected colon")

        operations = []
        while not self.match(TT.semicolon):
            operations.append(self.alu_operation())
            if not self.match(TT.semicolon):
                self.consume(TT.comma, "expected comma")

        self.consume(TT.semicolon, "expected semicolon")

        return ALUInstruction(
            tags,
            let_expressions,
            operations,
        )

    def tex_is_opcode(self):
        opcode_keywords = {
            KW.NOP, KW.LD, KW.TEXKILL, KW.PROJ,
            KW.LODBIAS, KW.LOD, KW.DXDY,
        }
        if self.match(TT.keyword):
            token = self.peek()
            return token.keyword in opcode_keywords
        return False

    def tex_operation(self):
        dest_addr_swizzles = []
        while not self.tex_is_opcode():
            dest_addr_swizzles.append(self.dest_addr_swizzle())
        opcode_keyword = self.consume(TT.keyword, "expected tex opcode keyword")

        self.consume_keyword(KW.TEX, "expected tex keyword")
        self.consume(TT.left_square, "expected left square")
        tex_id_identifier = self.consume(TT.identifier, "expected tex ID identifier")
        self.consume(TT.right_square, "expected left square")
        self.consume(TT.dot, "expected dot")
        tex_dst_swizzle_identifier = self.consume(TT.identifier, "expected src swizzle")

        self.consume_keyword(KW.TEMP, "expected temp keyword")
        self.consume(TT.left_square, "expected left square")
        tex_src_address_identifier = self.consume(TT.identifier, "expected tex address identifier")
        self.consume(TT.right_square, "expected left square")
        self.consume(TT.dot, "expected dot")
        tex_src_swizzle_identifier = self.consume(TT.identifier, "expected dst swizzle")

        return TEXOperation(
            dest_addr_swizzles,
            opcode_keyword,
            tex_id_identifier,
            tex_dst_swizzle_identifier,
            tex_src_address_identifier,
            tex_src_swizzle_identifier,
        )

    def tex_instruction(self):
        tags = list()
        tag_keywords = set([KW.OUT, KW.TEX_SEM_WAIT, KW.TEX_SEM_ACQUIRE, KW.NOP, KW.ALU_WAIT, KW.LAST])
        while True:
            if self.match(TT.keyword):
                token = self.peek()
                if token.keyword in tag_keywords:
                    self.advance()
                    tag_keywords.remove(token.keyword)
                    tags.append(token)
                    continue
            break

        operation = self.tex_operation()

        self.consume(TT.semicolon, "expected semicolon")

        return TEXInstruction(
            tags,
            operation,
        )

    def instruction(self):
        out = False
        if self.match_keyword(KW.TEX):
            self.advance()
            return self.tex_instruction()
        else:
            return self.alu_instruction(out=False)

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
