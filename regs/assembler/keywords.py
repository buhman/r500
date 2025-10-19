from dataclasses import dataclass
from typing import Optional
from enum import Enum, auto

@dataclass
class MVE:
    name: str
    synonym: Optional[str]
    value: int

@dataclass
class VE:
    name: str
    synonym: Optional[str]
    value: int

@dataclass
class ME:
    name: str
    synonym: Optional[str]
    value: int

macro_vector_operations = [
    MVE(b"MACRO_OP_2CLK_MADD"       , None      , 0),
    MVE(b"MACRO_OP_2CLK_M2X_ADD"    , None      , 1),
]

vector_operations = [
       # name                       synonym    value
    VE(b"VECTOR_NO_OP"              , b"VE_NOP" , 0),
    VE(b"VE_DOT_PRODUCT"            , b"VE_DOT" , 1),
    VE(b"VE_MULTIPLY"               , b"VE_MUL" , 2),
    VE(b"VE_ADD"                    , None      , 3),
    VE(b"VE_MULTIPLY_ADD"           , b"VE_MAD" , 4),
    VE(b"VE_DISTANCE_VECTOR"        , None      , 5),
    VE(b"VE_FRACTION"               , b"VE_FRC" , 6),
    VE(b"VE_MAXIMUM"                , b"VE_MAX" , 7),
    VE(b"VE_MINIMUM"                , b"VE_MIN" , 8),
    VE(b"VE_SET_GREATER_THAN_EQUAL" , b"VE_SGE" , 9),
    VE(b"VE_SET_LESS_THAN"          , b"VE_SLT" , 10),
    VE(b"VE_MULTIPLYX2_ADD"         , None      , 11),
    VE(b"VE_MULTIPLY_CLAMP"         , None      , 12),
    VE(b"VE_FLT2FIX_DX"             , None      , 13),
    VE(b"VE_FLT2FIX_DX_RND"         , None      , 14),
    VE(b"VE_PRED_SET_EQ_PUSH"       , None      , 15),
    VE(b"VE_PRED_SET_GT_PUSH"       , None      , 16),
    VE(b"VE_PRED_SET_GTE_PUSH"      , None      , 17),
    VE(b"VE_PRED_SET_NEQ_PUSH"      , None      , 18),
    VE(b"VE_COND_WRITE_EQ"          , None      , 19),
    VE(b"VE_COND_WRITE_GT"          , None      , 20),
    VE(b"VE_COND_WRITE_GTE"         , None      , 21),
    VE(b"VE_COND_WRITE_NEQ"         , None      , 22),
    VE(b"VE_COND_MUX_EQ"            , None      , 23),
    VE(b"VE_COND_MUX_GT"            , None      , 24),
    VE(b"VE_COND_MUX_GTE"           , None      , 25),
    VE(b"VE_SET_GREATER_THAN"       , b"VE_SGT" , 26),
    VE(b"VE_SET_EQUAL"              , b"VE_SEQ" , 27),
    VE(b"VE_SET_NOT_EQUAL"          , b"VE_SNE" , 28),
]

math_operations = [
       # name                       synonym    value
    ME(b"MATH_NO_OP"                , b"ME_NOP" , 0),
    ME(b"ME_EXP_BASE2_DX"           , b"ME_EXP" , 1),
    ME(b"ME_LOG_BASE2_DX"           , b"ME_LOG2", 2),
    ME(b"ME_EXP_BASEE_FF"           , b"ME_EXPE", 3),
    ME(b"ME_LIGHT_COEFF_DX"         , None      , 4),
    ME(b"ME_POWER_FUNC_FF"          , b"ME_POW" , 5),
    ME(b"ME_RECIP_DX"               , b"ME_RCP" , 6),
    ME(b"ME_RECIP_FF"               , None      , 7),
    ME(b"ME_RECIP_SQRT_DX"          , b"ME_RSQ" , 8),
    ME(b"ME_RECIP_SQRT_FF"          , None      , 9),
    ME(b"ME_MULTIPLY"               , b"ME_MUL" , 10),
    ME(b"ME_EXP_BASE2_FULL_DX"      , None      , 11),
    ME(b"ME_LOG_BASE2_FULL_DX"      , None      , 12),
    ME(b"ME_POWER_FUNC_FF_CLAMP_B"  , None      , 13),
    ME(b"ME_POWER_FUNC_FF_CLAMP_B1" , None      , 14),
    ME(b"ME_POWER_FUNC_FF_CLAMP_01" , None      , 15),
    ME(b"ME_SIN"                    , None      , 16),
    ME(b"ME_COS"                    , None      , 17),
    ME(b"ME_LOG_BASE2_IEEE"         , None      , 18),
    ME(b"ME_RECIP_IEEE"             , None      , 19),
    ME(b"ME_RECIP_SQRT_IEEE"        , None      , 20),
    ME(b"ME_PRED_SET_EQ"            , None      , 21),
    ME(b"ME_PRED_SET_GT"            , None      , 22),
    ME(b"ME_PRED_SET_GTE"           , None      , 23),
    ME(b"ME_PRED_SET_NEQ"           , None      , 24),
    ME(b"ME_PRED_SET_CLR"           , None      , 25),
    ME(b"ME_PRED_SET_INV"           , None      , 26),
    ME(b"ME_PRED_SET_POP"           , None      , 27),
    ME(b"ME_PRED_SET_RESTORE"       , None      , 28),
]

class KW(Enum):
    temporary = auto()
    a0 = auto()
    out = auto()
    out_repl_x = auto()
    alt_temporary = auto()
    input = auto()
    absolute = auto()
    relative_a0 = auto()
    relative_i0 = auto()
    constant = auto()

keywords = [
    (KW.temporary     , b"temporary"     , b"temp"),
    (KW.a0            , b"a0"            , None),
    (KW.out           , b"out"           , None),
    (KW.out_repl_x    , b"out_repl_x"    , None),
    (KW.alt_temporary , b"alt_temporary" , b"alt_temp"),
    (KW.input         , b"input"         , None),
    (KW.absolute      , b"absolute"      , None),
    (KW.relative_a0   , b"relative_a0"   , None),
    (KW.relative_i0   , b"relative_i0"   , None),
    (KW.constant      , b"constant"      , b"const"),
]

def find_keyword(b: memoryview):
    b = bytes(b)
    for vector_op in vector_operations:
        if vector_op.name == b.upper() or (vector_op.synonym is not None and vector_op.synonym == b.upper()):
            return vector_op
    for math_op in math_operations:
        if math_op.name == b.upper() or (math_op.synonym is not None and math_op.synonym == b.upper()):
            return math_op
    for keyword, name, synonym in keywords:
        if name == b.lower() or (synonym is not None and synonym == b.lower()):
            return keyword
    return None
