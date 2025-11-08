from dataclasses import dataclass
from typing import Union
from collections import OrderedDict
from enum import IntEnum

from assembler.validator import ValidatorError
from assembler.fs import parser
from assembler.fs.keywords import KW
from assembler.fs.common_validator import RGBMask, AlphaMask
from assembler.fs.common_validator import validate_identifier_number
from assembler.fs.common_validator import keywords_to_string

@dataclass
class Masks:
    alpha_wmask: AlphaMask
    rgb_wmask: RGBMask

class TEXOp(IntEnum):
    NOP = 0
    LD = 1
    TEXKILL = 2
    PROJ = 3
    LODBIAS = 4
    LOD = 5
    DXDY = 6

class Swizzle(IntEnum):
    R = 0
    G = 1
    B = 2
    A = 3

@dataclass
class Instruction:
    tags: set[Union[KW.TEX_SEM_WAIT, KW.TEX_SEM_ACQUIRE, KW.OUT]]
    masks: Masks
    opcode: TEXOp
    tex_id: int
    src_addr: int
    src_swizzle: tuple[Swizzle, Swizzle, Swizzle, Swizzle]
    dst_addr: int
    dst_swizzle: tuple[Swizzle, Swizzle, Swizzle, Swizzle]

def validate_swizzle(token):
    if len(token.lexeme) != 4:
        raise ValidatorError("invalid swizzle identifier", token)
    swizzles = OrderedDict([
        (ord(b"r"), Swizzle.R),
        (ord(b"g"), Swizzle.G),
        (ord(b"b"), Swizzle.B),
        (ord(b"a"), Swizzle.A),
    ])
    if not all(c in swizzles for c in token.lexeme):
        raise ValidatorError("invalid swizzle identifier", token)
    return tuple(swizzles[c] for c in token.lexeme)

def validate_mask_swizzle(token) -> tuple[AlphaMask, RGBMask]:
    rgba_masks = OrderedDict([
        (b"none" , (AlphaMask.NONE, RGBMask.NONE)),
        (b"r"    , (AlphaMask.NONE, RGBMask.R)),
        (b"g"    , (AlphaMask.NONE, RGBMask.G)),
        (b"rg"   , (AlphaMask.NONE, RGBMask.RG)),
        (b"b"    , (AlphaMask.NONE, RGBMask.B)),
        (b"rb"   , (AlphaMask.NONE, RGBMask.RB)),
        (b"gb"   , (AlphaMask.NONE, RGBMask.GB)),
        (b"rgb"  , (AlphaMask.NONE, RGBMask.RGB)),
        (b"ra"   , (AlphaMask.A, RGBMask.R)),
        (b"ga"   , (AlphaMask.A, RGBMask.G)),
        (b"rga"  , (AlphaMask.A, RGBMask.RG)),
        (b"ba"   , (AlphaMask.A, RGBMask.B)),
        (b"rba"  , (AlphaMask.A, RGBMask.RB)),
        (b"gba"  , (AlphaMask.A, RGBMask.GB)),
        (b"rgba" , (AlphaMask.A, RGBMask.RGB)),
    ])
    if token.lexeme not in rgba_masks:
        raise ValidatorError("invalid destination mask", token)
    return rgba_masks[token.lexeme]

def validate_masks(ins_ast: parser.TEXInstruction):
    addresses = set()
    dests = set()

    masks = Masks(
        alpha_wmask = AlphaMask.NONE,
        rgb_wmask = RGBMask.NONE,
    )

    for dest_addr_swizzle in ins_ast.operation.dest_addr_swizzles:
        dest_keyword = dest_addr_swizzle.dest_keyword
        if dest_keyword.keyword is not KW.TEMP:
            raise ValidatorError(f"invalid dest keyword, expected `temp`", dest_keyword)
        dests.add(dest_keyword.keyword)

        addr_identifier = dest_addr_swizzle.addr_identifier
        addr = validate_identifier_number(addr_identifier)
        addresses.add(addr)

        swizzle_identifier = dest_addr_swizzle.swizzle_identifier
        alpha_mask, rgb_mask = validate_mask_swizzle(swizzle_identifier)

        masks.alpha_wmask = alpha_mask
        masks.rgb_wmask = rgb_mask

    if len(addresses) > 1:
        raise ValidatorError("contradictory destination address", ins_ast.operation.dest_addr_swizzles[-1].addr_identifier)

    address, = addresses

    return masks, address

op_kws = OrderedDict([
    (KW.NOP, TEXOp.NOP),
    (KW.LD, TEXOp.LD),
    (KW.TEXKILL, TEXOp.TEXKILL),
    (KW.PROJ, TEXOp.PROJ),
    (KW.LODBIAS, TEXOp.LODBIAS),
    (KW.LOD, TEXOp.LOD),
    (KW.DXDY, TEXOp.DXDY),
])

def validate_opcode(token):
    if token.keyword not in op_kws:
        raise ValidatorError("invalid tex opcode keyword", token)
    return op_kws[token.keyword]

def validate_instruction(ins_ast: parser.TEXInstruction):
    tags = set([tag.keyword for tag in ins_ast.tags])

    masks, dst_addr = validate_masks(ins_ast)
    opcode = validate_opcode(ins_ast.operation.opcode_keyword)

    tex_id = validate_identifier_number(ins_ast.operation.tex_id_identifier)

    dst_swizzle = validate_swizzle(ins_ast.operation.tex_dst_swizzle_identifier)

    src_address = validate_identifier_number(ins_ast.operation.tex_src_address_identifier)
    src_swizzle = validate_swizzle(ins_ast.operation.tex_src_swizzle_identifier)

    return Instruction(
        tags = tags,
        masks = masks,
        opcode = opcode,
        tex_id = tex_id,
        src_addr = src_address,    # texture coordinate address
        src_swizzle = src_swizzle, # texture coordinate swizzle
        dst_addr = dst_addr,       # temp address
        dst_swizzle = dst_swizzle, # texture value swizzle
    )
