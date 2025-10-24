from dataclasses import dataclass
from enum import IntEnum
from itertools import pairwise
from typing import Union

from assembler.lexer import Token
from assembler.validator import ValidatorError
from assembler.vs.keywords import KW
from assembler.vs import opcodes
from collections import OrderedDict


class SourceType(IntEnum):
    temporary = 0
    input = 1
    constant = 2
    alt_temporary = 3

class SwizzleSelect(IntEnum):
    x = 0
    y = 1
    z = 2
    w = 3
    zero = 4
    one = 5
    half = 6
    unused = 7

@dataclass
class Source:
    type: SourceType
    absolute: bool
    offset: int
    swizzle_selects: tuple[SwizzleSelect, SwizzleSelect, SwizzleSelect, SwizzleSelect]
    modifiers: tuple[bool, bool, bool, bool]

class DestinationType(IntEnum):
    temporary = 0
    a0 = 1
    out = 2
    out_repl_x = 3
    alt_temporary = 4
    input = 5

@dataclass
class Destination:
    type: DestinationType
    offset: int
    write_enable: tuple[bool, bool, bool, bool]

@dataclass
class OpcodeDestination:
    macro_inst: bool
    reg_type: int

@dataclass
class Instruction:
    destination: Destination
    saturation: bool
    sources: list[Source]
    opcode: Union[opcodes.VE, opcodes.ME, opcodes.MVE]

def validate_opcode(opcode_keyword: Token):
    if type(opcode_keyword.keyword) is opcodes.ME:
        return opcode_keyword.keyword
    elif type(opcode_keyword.keyword) is opcodes.VE:
        return opcode_keyword.keyword
    else:
        raise ValidatorError("invalid opcode keyword", opcode_keyword)

def validate_identifier_number(token):
    try:
        return int(token.lexeme, 10)
    except ValueError:
        raise ValidatorError("invalid number", token)

def validate_destination_write_enable(write_enable_identifier):
    we_chars_s = b"xyzw"
    we_chars = {c: i for i, c in enumerate(we_chars_s)}
    we = bytes(write_enable_identifier.lexeme).lower()

    if not all(c in we_chars for c in we):
        raise ParserError("invalid character in destination write enable", write_enable_identifier)
    if not all(we_chars[a] < we_chars[b] for a, b in pairwise(we)) or len(set(we)) != len(we):
        raise ParserError("misleading non-sequential destination write enable", write_enable_identifier)

    we = set(we)
    return tuple(c in we for c in we_chars_s)

def validate_destination(destination):
    destination_type_keywords = OrderedDict([
        (KW.temporary     , (DestinationType.temporary    , 128)), # 32
        (KW.a0            , (DestinationType.a0           , 1  )), # ??
        (KW.out           , (DestinationType.out          , 128)), # ?
        (KW.out_repl_x    , (DestinationType.out_repl_x   , 128)), # ?
        (KW.alt_temporary , (DestinationType.alt_temporary, 20 )), # 20
        (KW.input         , (DestinationType.input        , 128)), # 32
    ])
    if destination.type_keyword.keyword not in destination_type_keywords:
        raise ValidatorError("invalid destination type keyword", destination.type_keyword)
    type, max_offset = destination_type_keywords[destination.type_keyword.keyword]
    offset = validate_identifier_number(destination.offset_identifier)
    if offset >= max_offset:
        raise ValidatorError("invalid offset value", source.offset_identifier)

    write_enable = validate_destination_write_enable(destination.write_enable_identifier)

    return Destination(
        type,
        offset,
        write_enable
    )

def parse_swizzle_lexeme(token):
    swizzle_select_characters = OrderedDict([
        (ord(b"x"), SwizzleSelect.x),
        (ord(b"y"), SwizzleSelect.y),
        (ord(b"z"), SwizzleSelect.z),
        (ord(b"w"), SwizzleSelect.w),
        (ord(b"0"), SwizzleSelect.zero),
        (ord(b"1"), SwizzleSelect.one),
        (ord(b"h"), SwizzleSelect.half),
        (ord(b"_"), SwizzleSelect.unused),
    ])
    lexeme = bytes(token.lexeme).lower()
    swizzles = []
    modifier = False
    for c in lexeme:
        if c == ord(b"-"):
            modifier = True
        else:
            if c not in swizzle_select_characters:
                raise ValueError(c)
            swizzles.append((swizzle_select_characters[c], modifier))
            modifier = False
    return tuple(zip(*swizzles))

def validate_source(source):
    source_type_keywords = OrderedDict([
        (KW.temporary     , (SourceType.temporary    , 128)), # 32
        (KW.input         , (SourceType.input        , 128)), # 32
        (KW.constant      , (SourceType.constant     , 256)), # 256
        (KW.alt_temporary , (SourceType.alt_temporary,  20)), # 20
    ])
    if source.type_keyword.keyword not in source_type_keywords:
        raise ValidatorError("invalid source type keyword", source.type_keyword)

    type, max_offset = source_type_keywords[source.type_keyword.keyword]
    absolute = source.absolute
    offset = validate_identifier_number(source.offset_identifier)
    if offset >= max_offset:
        raise ValidatorError("invalid offset value", source.offset_identifier)
    try:
        swizzle_selects, modifiers = parse_swizzle_lexeme(source.swizzle_identifier)
    except ValueError:
        raise ValidatorError("invalid source swizzle", source.swizzle_identifier)

    return Source(
        type,
        absolute,
        offset,
        swizzle_selects,
        modifiers
    )

def addresses_by_type(sources, source_type):
    return set(int(source.offset)
               for source in sources
               if source.type == source_type)

def source_ix_with_type_reversed(sources, source_type):
    for i, source in reversed(list(enumerate(sources))):
        if source.type == source_type:
            return i
    assert False, (sources, source_type)

def validate_source_address_counts(sources_ast, sources, opcode):
    temporary_address_count = len(addresses_by_type(sources, SourceType.temporary))
    assert temporary_address_count >= 0 and temporary_address_count <= 3
    assert type(opcode) in {opcodes.VE, opcodes.ME}
    if temporary_address_count == 3:
        if opcode == opcodes.VE_MULTIPLY_ADD:
            opcode = opcodes.MACRO_OP_2CLK_MADD
        elif opcode == opcodes.VE_MULTIPLYX2_ADD:
            opcode = opcodes.MACRO_OP_2CLK_M2X_ADD
        else:
            raise ValidatorError("too many temporary addresses in non-macro operation(s)",
                                 sources_ast[-1].offset_identifier)

    constant_count = len(addresses_by_type(sources, SourceType.constant))
    if constant_count > 1:
        source_ix = source_ix_with_type_reversed(sources, SourceType.constant)
        raise ValidatorError(f"too many constant addresses in operation(s); expected 1, have {constant_count}",
                             sources_ast[source_ix].offset_identifier)

    input_count = len(addresses_by_type(sources, SourceType.input))
    if input_count > 1:
        source_ix = source_with_type_reversed(sources, SourceType.input)
        raise ValidatorError(f"too many input addresses in operation(s); expected 1, have {input_count}",
                             sources_ast[source_ix].offset_identifier)

    alt_temporary_count = len(addresses_by_type(sources, SourceType.alt_temporary))
    if alt_temporary_count > 1:
        source_ix = source_with_type_reversed(sources, SourceType.alt_temporary)
        raise ValidatorError(f"too many alt temporary addresses in operation(s); expected 1, have {alt_temporary_count}",
                             sources_ast[source_ix].offset_identifier)

    return opcode

def validate_instruction_inner(operation, opcode):
    destination = validate_destination(operation.destination)
    saturation = False
    if operation.opcode_suffix_keyword is not None:
        if operation.opcode_suffix_keyword.keyword is not KW.saturation:
            raise ValidatorError("invalid opcode saturation suffix", operation.opcode_suffix_keyword)
        saturation = True
    if len(operation.sources) > 3:
        raise ValidatorError("too many sources in operation", operation.sources[0].type_keyword)
    if len(operation.sources) != opcode.operand_count:
        raise ValidatorError(f"incorrect number of source operands; expected {opcode.operand_count}", operation.sources[0].type_keyword)

    sources = []
    for source in operation.sources:
        sources.append(validate_source(source))
    opcode = validate_source_address_counts(operation.sources, sources, opcode)

    return Instruction(
        destination,
        saturation,
        sources,
        opcode
    )

def validate_instruction(ins):
    if len(ins.operations) > 2:
        raise ValidatorError("too many operations in instruction", ins.operations[0].destination.type_keyword)

    opcodes = [validate_opcode(operation.opcode_keyword) for operation in ins.operations]
    opcode_types = set(type(opcode) for opcode in opcodes)
    if len(opcode_types) != len(opcodes):
        opcode_type, = opcode_types
        raise ValidatorError(f"invalid dual math operation: too many opcodes of type {opcode_type}", ins.operations[0].opcode_keyword)

    if len(opcodes) == 2:
        assert False, "not implemented"
        #return validate_dual_math_instruction(ins, opcodes)
    else:
        assert len(opcodes) == 1
        return validate_instruction_inner(ins.operations[0], opcodes[0])

if __name__ == "__main__":
    from assembler.lexer import Lexer
    from assembler.parser import ParserError
    from assembler.vs.parser import Parser
    from assembler.vs.keywords import find_keyword
    from assembler.error import print_error
    buf = b"""
  out[0].xz    = VE_MAD.SAT    |temp[1].-y-_0-_| const[2].x_0_ const[2].x_0_ ;
"""
#atemp[0].xz    = ME_SIN    input[0].-y-_-0-_ ;

    lexer = Lexer(buf, find_keyword, emit_newlines=False, minus_is_token=False)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    from pprint import pprint
    try:
        ins = parser.instruction()
        pprint(validate_instruction(ins))
    except ValidatorError as e:
        print_error(None, buf, e)
        raise
    except ParserError as e:
        print_error(None, buf, e)
        raise
