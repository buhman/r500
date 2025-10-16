from dataclasses import dataclass
from enum import Enum, auto
from itertools import chain
from typing import Union

from assembler import keywords

DEBUG = True

class TT(Enum):
    eof = auto()
    eol = auto()
    left_square = auto()
    right_square = auto()
    left_paren = auto()
    right_paren = auto()
    equal = auto()
    dot = auto()
    identifier = auto()
    keyword = auto()

@dataclass
class Token:
    line: int
    col: int
    type: TT
    lexeme: memoryview
    keyword: Union[keywords.VE, keywords.ME, keywords.KW] = None

identifier_characters = set(chain(
    b'abcdefghijklmnopqrstuvwxyz'
    b'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
    b'0123456789',
    b'_-'
))

class LexerError(Exception):
    pass

class Lexer:
    def __init__(self, buf: memoryview):
        self.start_ix = 0
        self.current_ix = 0
        self.buf = memoryview(buf)
        self.line = 1
        self.col = 0

    def at_end_p(self):
        return self.current_ix >= len(self.buf)

    def lexeme(self):
        if DEBUG:
            return bytes(self.buf[self.start_ix:self.current_ix])
        else:
            return memoryview(self.buf[self.start_ix:self.current_ix])

    def advance(self):
        c = self.buf[self.current_ix]
        self.col += 1
        self.current_ix += 1
        return c

    def peek(self):
        return self.buf[self.current_ix]

    def pos(self):
        return self.line, self.col - (self.current_ix - self.start_ix)

    def identifier(self):
        while not self.at_end_p() and self.peek() in identifier_characters:
            self.advance()
        keyword = keywords.find_keyword(self.lexeme())
        if keyword is not None:
            return Token(*self.pos(), TT.keyword, self.lexeme(), keyword)
        else:
            return Token(*self.pos(), TT.identifier, self.lexeme(), None)

    def lex_token(self):
        while True:
            self.start_ix = self.current_ix

            if self.at_end_p():
                return Token(*self.pos(), TT.eof, self.lexeme())

            c = self.advance()
            if c == ord('('):
                return Token(*self.pos(), TT.left_paren, self.lexeme())
            elif c == ord(')'):
                return Token(*self.pos(), TT.right_paren, self.lexeme())
            elif c == ord('['):
                return Token(*self.pos(), TT.left_square, self.lexeme())
            elif c == ord(']'):
                return Token(*self.pos(), TT.right_square, self.lexeme())
            elif c == ord('='):
                return Token(*self.pos(), TT.equal, self.lexeme())
            elif c == ord('.'):
                return Token(*self.pos(), TT.dot, self.lexeme())
            elif c == ord(';'):
                while not at_end_p() and peek() != ord('\n'):
                    self.advance()
            elif c == ord(' ') or c == ord('\r') or c == ord('\t'):
                pass
            elif c == ord('\n'):
                pos = self.pos()
                self.line += 1
                self.col = 0
                return Token(*pos, TT.eol, self.lexeme())
            elif c in identifier_characters:
                return self.identifier()
            else:
                raise LexerError(f"unexpected character at line:{self.line} col:{self.col}")

    def lex_tokens(self):
        while True:
            token = self.lex_token()
            yield token
            if token.type is TT.eof:
                break

if __name__ == "__main__":
    test = b"out[0].xz    = VE_MAD    input[0].-y-_-0-_ temp[0].x_0_      temp[0].y_0_"
    lexer = Lexer(test)
    for token in lexer.lex_tokens():
        print(token)
