from typing import Any

from assembler.lexer import TT

class ParserError(Exception):
    pass

class BaseParser:
    def __init__(self, tokens: list[Any]):
        self.current_ix = 0
        self.tokens = tokens

    def peek(self, offset=0):
        token = self.tokens[self.current_ix + offset]
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

    def match_keyword(self, keyword):
        if self.match(TT.keyword):
            token = self.peek()
            return token.keyword == keyword
        else:
            return False

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
