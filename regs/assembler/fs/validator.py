from pprint import pprint
from dataclasses import dataclass
from enum import Enum, IntEnum, auto
from collections import OrderedDict

from assembler.fs.parser import Mod
from assembler.fs.keywords import _keyword_to_string, KW
from assembler.error import print_error

class ValidatorError(Exception):
    pass

class SrcAddrType(Enum):
    temp = auto()
    const = auto()
    float = auto()

@dataclass
class SrcAddr:
    type: SrcAddrType
    value: int

class SrcMod(IntEnum):
    nop = 0
    neg = 1
    abs = 2
    nab = 3

class SrcpOp(IntEnum):
    neg2 = 0
    sub = 1
    add = 2
    neg = 3

@dataclass
class Addr:
    src0: SrcAddr = None
    src1: SrcAddr = None
    src2: SrcAddr = None
    srcp: SrcpOp = None

@dataclass
class AddrRGBAlpha:
    alpha: Addr
    rgb: Addr

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

class Unit(Enum):
    alpha = auto()
    rgb = auto()

class RGBOp(IntEnum):
    MAD = 0
    DP3 = 1
    DP4 = 2
    D2A = 3
    MIN = 4
    MAX = 5
    CND = 7
    CMP = 8
    FRC = 9
    SOP = 10
    MDH = 11
    MDV = 12

class AlphaOp(IntEnum):
    MAD = 0
    DP = 1
    MIN = 2
    MAX = 3
    CND = 5
    CMP = 6
    FRC = 7
    EX2 = 8
    LN2 = 9
    RCP = 10
    RSQ = 11
    SIN = 12
    COS = 13
    MDH = 14
    MDV = 15

@dataclass
class RGBDest:
    addrd: int
    wmask: RGBMask
    omask: RGBMask

@dataclass
class AlphaDest:
    addrd: int
    wmask: AlphaMask
    omask: AlphaMask

class SwizzleSelSrc(IntEnum):
    src0 = 0
    src1 = 1
    src2 = 2
    srcp = 3

class Swizzle(IntEnum):
    r = 0
    g = 1
    b = 2
    a = 3
    zero = 4
    half = 5
    one = 6
    unused = 7

@dataclass
class SwizzleSel:
    src: SwizzleSelSrc
    swizzle: list[Swizzle]
    mod: Mod

@dataclass
class AlphaOperation:
    dest: AlphaDest
    opcode: AlphaOp
    sels: list[SwizzleSel]

@dataclass
class RGBOperation:
    dest: RGBDest
    opcode: RGBOp
    sels: list[SwizzleSel]

class InstructionType(IntEnum):
    ALU = 0
    OUT = 1
    FC = 2
    TEX = 3

@dataclass
class Instruction:
    type: InstructionType
    tex_sem_wait: bool
    addr: Addr
    alpha_op: AlphaOperation
    rgb_op: RGBOperation

def validate_identifier_number(token):
    try:
        return int(token.lexeme, 10)
    except ValueError:
        raise ValidatorError("invalid number", token)

def keywords_to_string(keywords):
    return [_keyword_to_string[s].decode("utf-8") for s in keywords]

def validate_instruction_let_expressions(let_expressions):
    src_keywords = [KW.SRC0, KW.SRC1, KW.SRC2, KW.SRCP]
    src_keyword_strs = keywords_to_string(src_keywords)
    rgb_alpha_swizzles = [b"rgb", b"a"]

    addr_rgb_alpha = AddrRGBAlpha(Addr(), Addr())

    def set_src_by_keyword(addr, keyword, value):
        if keyword == KW.SRC0:
            addr.src0 = value
        elif keyword == KW.SRC1:
            addr.src1 = value
        elif keyword == KW.SRC2:
            addr.src2 = value
        elif keyword == KW.SRCP:
            addr.srcp = value
        else:
            assert False, keyword

    def src_value(expr, src):
        if src in {KW.SRC0, KW.SRC1, KW.SRC2}:
            keyword_to_src_addr_type = OrderedDict([
                (KW.TEMP, SrcAddrType.temp),
                (KW.CONST, SrcAddrType.const),
                (KW.FLOAT, SrcAddrType.float),
            ])
            src_addr_type_strs = keywords_to_string(keyword_to_src_addr_type.keys())
            type_kw = expr.addr_keyword.keyword
            if type_kw not in keyword_to_src_addr_type:
                raise ValidatorError(f"invalid src addr type, expected one of {src_addr_type_strs}", expr.addr_keyword)

            type = keyword_to_src_addr_type[type_kw]
            value = validate_identifier_number(expr.addr_value_identifier)
            if type is SrcAddrType.float:
                if value >= 128:
                    raise ValidatorError(f"invalid float value", expr.addr_value_identifier)
            elif type is SrcAddrType.temp:
                if value >= 128:
                    raise ValidatorError(f"invalid temp value", expr.addr_value_identifier)
            elif type is SrcAddrType.const:
                if value >= 256:
                    raise ValidatorError(f"invalid const value", expr.addr_value_identifier)
            else:
                assert False, (id(type), id(SrcAddrType.float))

            return SrcAddr(
                type,
                value,
            )
        elif src == KW.SRCP:
            keyword_to_srcp_op = OrderedDict([
                (KW.NEG2, SrcpOp.neg2),
                (KW.SUB, SrcpOp.sub),
                (KW.ADD, SrcpOp.add),
                (KW.NEG, SrcpOp.neg),
            ])
            srcp_op_strs = keywords_to_string(keyword_to_srcp_op.keys())
            op = expr.addr_keyword.keyword
            if op not in keyword_to_srcp_op:
                raise ValidatorError(f"invalid srcp op, expected one of {srcp_op_strs}", expr.addr_keyword)
            return keyword_to_srcp_op[op]
        else:
            assert False, src

    sources = set()

    for expr in let_expressions:
        src = expr.src_keyword.keyword
        if src not in src_keywords:
            raise ValidatorError(f"invalid src keyword, expected one of {src_keyword_strs}", expr.src_keyword)

        src_swizzle = expr.src_swizzle_identifier.lexeme.lower()
        if src_swizzle not in rgb_alpha_swizzles:
            raise ValidatorError(f"invalid src swizzle, expected one of {rgb_alpha_swizzles}", expr.src_swizzle_identifier)

        source = (_keyword_to_string[src].lower(), src_swizzle)
        if source in sources:
            raise ValidatorError(f"duplicate source/swizzle in let expressions", expr.src_swizzle_identifier)
        sources.add(source)

        value = src_value(expr, src)

        if src_swizzle == b"a":
            set_src_by_keyword(addr_rgb_alpha.alpha, src, value)
        elif src_swizzle == b"rgb":
            set_src_by_keyword(addr_rgb_alpha.rgb, src, value)
        else:
            assert False, src_swizzle

    return addr_rgb_alpha

def prevalidate_mask(dest_addr_swizzle, valid_masks):
    # we don't know yet whether this is an Alpha operation or an RGB operation
    swizzle_str = dest_addr_swizzle.swizzle_identifier.lexeme
    if swizzle_str.lower() not in valid_masks:
        raise ValidatorError(f"invalid write mask, expected one of {valid_masks}", dest_addr_swizzle.swizzle_identifier)

    mask = swizzle_str.lower()
    return mask

rgb_op_kws = OrderedDict([
    (KW.MAD, RGBOp.MAD),
    (KW.DP3, RGBOp.DP3),
    (KW.DP4, RGBOp.DP4),
    (KW.D2A, RGBOp.D2A),
    (KW.MIN, RGBOp.MIN),
    (KW.MAX, RGBOp.MAX),
    (KW.CND, RGBOp.CND),
    (KW.CMP, RGBOp.CMP),
    (KW.FRC, RGBOp.FRC),
    (KW.SOP, RGBOp.SOP),
    (KW.MDH, RGBOp.MDH),
    (KW.MDV, RGBOp.MDV)
])
alpha_op_kws = OrderedDict([
    (KW.MAD, AlphaOp.MAD),
    (KW.DP, AlphaOp.DP),
    (KW.MIN, AlphaOp.MIN),
    (KW.MAX, AlphaOp.MAX),
    (KW.CND, AlphaOp.CND),
    (KW.CMP, AlphaOp.CMP),
    (KW.FRC, AlphaOp.FRC),
    (KW.EX2, AlphaOp.EX2),
    (KW.LN2, AlphaOp.LN2),
    (KW.RCP, AlphaOp.RCP),
    (KW.RSQ, AlphaOp.RSQ),
    (KW.SIN, AlphaOp.SIN),
    (KW.COS, AlphaOp.COS),
    (KW.MDH, AlphaOp.MDH),
    (KW.MDV, AlphaOp.MDV)
])

rgb_masks = OrderedDict([
    (b"none" , RGBMask.NONE),
    (b"r"    , RGBMask.R),
    (b"g"    , RGBMask.G),
    (b"rg"   , RGBMask.RG),
    (b"b"    , RGBMask.B),
    (b"rb"   , RGBMask.RB),
    (b"gb"   , RGBMask.GB),
    (b"rgb"  , RGBMask.RGB),
])

alpha_masks = OrderedDict([
    (b"none" , AlphaMask.NONE),
    (b"a"    , AlphaMask.A),
])

alpha_only_ops = set(alpha_op_kws.keys()) - set(rgb_op_kws.keys())
rgb_only_ops = set(rgb_op_kws.keys()) - set(alpha_op_kws.keys())
all_ops = set(rgb_op_kws.keys()) | set(alpha_op_kws.keys())

alpha_only_masks = set(alpha_masks.keys()) - set(rgb_masks.keys())
rgb_only_masks = set(rgb_masks.keys()) - set(alpha_masks.keys())
all_masks = set(rgb_masks.keys()) | set(alpha_masks.keys())

def infer_operation_units(operations):
    if len(operations) > 2:
        raise ValidatorError("too many operations in instruction", operations[-1].opcode_keyword)

    units = [None, None]
    for i, operation in enumerate(operations):
        opcode = operation.opcode_keyword.keyword
        if opcode not in all_ops:
            raise ValidatorError(f"invalid opcode keyword, expected one of {all_ops}", operation.opcode_keyword)

        if len(operation.dest_addr_swizzles) > 2:
            raise ValidationError("too many destinations in instruction", operation.dest_addr_swizzles[-1])

        masks = set(prevalidate_mask(dest_addr_swizzle, all_masks) for dest_addr_swizzle in operation.dest_addr_swizzles)

        def infer_opcode_unit():
            if opcode in alpha_only_ops:
                return Unit.alpha
            if opcode in rgb_only_ops:
                return Unit.rgb
            return None

        def infer_mask_unit():
            if any(mask in alpha_only_masks for mask in masks):
                return Unit.alpha
            if any(mask in rgb_only_masks for mask in masks):
                return Unit.rgb
            return None

        opcode_unit = infer_opcode_unit()
        mask_unit = infer_mask_unit()
        if opcode_unit is not None and mask_unit is not None and opcode_unit != mask_unit:
            raise ValidatorError(f"contradictory {mask_unit.name} write mask for {opcode_unit.name} opcode", operation.opcode_keyword)
        units[i] = opcode_unit or mask_unit

    if units[0] == units[1]:
        raise ValidatorError(f"invalid duplicate use of {units[1].name} unit", operations[1].opcode_keyword)

    other_unit = {
        Unit.alpha: Unit.rgb,
        Unit.rgb: Unit.alpha,
    }

    if units[0] is None:
        units[0] = other_unit[units[1]]
    if units[1] is None:
        units[1] = other_unit[units[0]]

    assert units[0] is not None
    assert units[1] is not None
    assert units[0] != units[1]

    for i, operation in enumerate(operations):
        yield units[i], operation

def validate_dest_keyword(dest_keyword):
    dest_keywords = [KW.OUT, KW.TEMP]
    dest_keyword_strs = keywords_to_string(dest_keywords)
    dest = dest_keyword.keyword
    if dest not in dest_keywords:
        raise ValidatorError(f"invalid dest keyword, expected one of {dest_keyword_strs}", dest_addr_swizzle.dest_keyword)
    return dest

def validate_instruction_operation_dest(dest_addr_swizzles, mask_lookup, type_cls):
    addrs = set()
    wmask = None
    omask = None
    for dest_addr_swizzle in dest_addr_swizzles:
        dest = validate_dest_keyword(dest_addr_swizzle.dest_keyword)
        addr = validate_identifier_number(dest_addr_swizzle.addr_identifier)
        mask = mask_lookup[dest_addr_swizzle.swizzle_identifier.lexeme.lower()]
        addrs.add(addr)
        if dest == KW.OUT:
            omask = mask
        elif dest == KW.TEMP:
            wmask = mask
        else:
            assert False, dest
    if len(addrs) > 1:
        raise ValidatorError(f"too many destination addresses", operation.dest_addr_swizzles[-1].addr_identifier)
    addrd, = addrs if addrs else [0]
    return type_cls(
        addrd=addrd,
        wmask=wmask,
        omask=omask
    )

swizzle_sel_src_kws = OrderedDict([
    (KW.SRC0, SwizzleSelSrc.src0),
    (KW.SRC1, SwizzleSelSrc.src1),
    (KW.SRC2, SwizzleSelSrc.src2),
    (KW.SRCP, SwizzleSelSrc.srcp),
])

swizzle_kws = OrderedDict([
    (ord("r"), Swizzle.r),
    (ord("g"), Swizzle.g),
    (ord("b"), Swizzle.b),
    (ord("a"), Swizzle.a),
    (ord("0"), Swizzle.zero),
    (ord("h"), Swizzle.half),
    (ord("1"), Swizzle.one),
    (ord("_"), Swizzle.unused),
])

def validate_instruction_operation_sels(swizzle_sels, is_alpha):
    if len(swizzle_sels) > 3:
        raise ValidatorError("too many swizzle sels", swizzle_sels[-1].sel_keyword)

    sels = []
    for swizzle_sel in swizzle_sels:
        if swizzle_sel.sel_keyword.keyword not in swizzle_sel_src_kws:
            raise ValidatorError("invalid swizzle src", swizzle_sel.sel_keyword.keyword)
        src = swizzle_sel_src_kws[swizzle_sel.sel_keyword.keyword]

        swizzle_lexeme = swizzle_sel.swizzle_identifier.lexeme.lower()
        swizzles_length = 1 if is_alpha else 3
        if len(swizzle_lexeme) != swizzles_length:
            raise ValidatorError("invalid swizzle length", swizzle_sel.swizzle_identifier)
        if not all(c in swizzle_kws for c in swizzle_lexeme):
            raise ValidatorError("invalid swizzle characters", swizzle_sel.swizzle_identifier)
        swizzle = [
            swizzle_kws[c] for c in swizzle_lexeme
        ]

        mod = swizzle_sel.mod
        sels.append(SwizzleSel(src, swizzle, mod))
    return sels

def validate_alpha_instruction_operation(operation):
    dest = validate_instruction_operation_dest(operation.dest_addr_swizzles,
                                               mask_lookup=alpha_masks,
                                               type_cls=AlphaDest)
    opcode = alpha_op_kws[operation.opcode_keyword.keyword]
    sels = validate_instruction_operation_sels(operation.swizzle_sels, is_alpha=True)
    return AlphaOperation(
        dest,
        opcode,
        sels
    )

def validate_rgb_instruction_operation(operation):
    dest = validate_instruction_operation_dest(operation.dest_addr_swizzles,
                                               mask_lookup=rgb_masks,
                                               type_cls=RGBDest)
    opcode = rgb_op_kws[operation.opcode_keyword.keyword]
    sels = validate_instruction_operation_sels(operation.swizzle_sels, is_alpha=False)
    return RGBOperation(
        dest,
        opcode,
        sels
    )

def validate_instruction_operations(operations):
    for unit, operation in infer_operation_units(operations):
        if unit is Unit.alpha:
            yield validate_alpha_instruction_operation(operation)
        elif unit is Unit.rgb:
            yield validate_rgb_instruction_operation(operation)
        else:
            assert False, unit

def validate_instruction(ins):
    addr_rgb_alpha = validate_instruction_let_expressions(ins.let_expressions)

    instruction_type = InstructionType.OUT if ins.out else InstructionType.ALU
    tex_sem_wait = ins.tex_sem_wait

    instruction = Instruction(
        instruction_type,
        tex_sem_wait,
        addr_rgb_alpha,
        None,
        None
    )
    for op in validate_instruction_operations(ins.operations):
        if type(op) is RGBOperation:
            instruction.rgb_op = op
        elif type(op) is AlphaOperation:
            instruction.alpha_op = op
        else:
            assert False, op
    return instruction

if __name__ == "__main__":
    from assembler.lexer import Lexer, LexerError
    from assembler.fs.parser import Parser, ParserError
    from assembler.fs.keywords import find_keyword

    buf = b"""
src0.a = float(0), src0.rgb = temp[0] , srcp.a = neg :
  out[0].none = temp[0].none = MAD src0.r src0.r src0.r ,
  out[0].none = temp[0].r    = DP3 src0.rg0 src0.rg0 ;
    """
    lexer = Lexer(buf, find_keyword, emit_newlines=False, minus_is_token=True)
    tokens = list(lexer.lex_tokens())
    parser = Parser(tokens)
    try:
        ins_ast = parser.instruction()
        pprint(validate_instruction(ins_ast))
    except LexerError as e:
        print_error(None, buf, e)
        raise
    except ParserError as e:
        print_error(None, buf, e)
        raise
    except ValidatorError as e:
        print_error(None, buf, e)
        raise
