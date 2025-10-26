from enum import IntEnum
from collections import OrderedDict

from assembler.fs.keywords import _keyword_to_string
from assembler.fs.keywords import KW
from assembler.validator import ValidatorError

class InstructionType(IntEnum):
    ALU = 0
    OUT = 1
    FC = 2
    TEX = 3

class RGBMask(IntEnum):
    NONE = 0
    R = 1
    G = 2
    RG = 3
    B = 4
    RB = 5
    GB = 6
    RGB = 7

class AlphaMask(IntEnum):
    NONE = 0
    A = 1

def validate_identifier_number(token):
    try:
        return int(token.lexeme, 10)
    except ValueError:
        raise ValidatorError("invalid number", token)

def validate_dest_keyword(dest_keyword):
    dest_keywords = [KW.OUT, KW.TEMP]
    dest_keyword_strs = keywords_to_string(dest_keywords)
    dest = dest_keyword.keyword
    if dest not in dest_keywords:
        raise ValidatorError(f"invalid dest keyword, expected one of {dest_keyword_strs}", dest_addr_swizzle.dest_keyword)
    return dest

def keywords_to_string(keywords):
    return [_keyword_to_string[s].decode("utf-8") for s in keywords]
