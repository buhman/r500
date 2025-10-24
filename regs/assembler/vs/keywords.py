from dataclasses import dataclass
from typing import Optional
from enum import Enum, auto

from assembler.vs import opcodes

operations = [
    opcodes.VECTOR_NO_OP,
    opcodes.VE_DOT_PRODUCT,
    opcodes.VE_MULTIPLY,
    opcodes.VE_ADD,
    opcodes.VE_MULTIPLY_ADD,
    opcodes.VE_DISTANCE_VECTOR,
    opcodes.VE_FRACTION,
    opcodes.VE_MAXIMUM,
    opcodes.VE_MINIMUM,
    opcodes.VE_SET_GREATER_THAN_EQUAL,
    opcodes.VE_SET_LESS_THAN,
    opcodes.VE_MULTIPLYX2_ADD,
    opcodes.VE_MULTIPLY_CLAMP,
    opcodes.VE_FLT2FIX_DX,
    opcodes.VE_FLT2FIX_DX_RND,
    opcodes.VE_PRED_SET_EQ_PUSH,
    opcodes.VE_PRED_SET_GT_PUSH,
    opcodes.VE_PRED_SET_GTE_PUSH,
    opcodes.VE_PRED_SET_NEQ_PUSH,
    opcodes.VE_COND_WRITE_EQ,
    opcodes.VE_COND_WRITE_GT,
    opcodes.VE_COND_WRITE_GTE,
    opcodes.VE_COND_WRITE_NEQ,
    opcodes.VE_COND_MUX_EQ,
    opcodes.VE_COND_MUX_GT,
    opcodes.VE_COND_MUX_GTE,
    opcodes.VE_SET_GREATER_THAN,
    opcodes.VE_SET_EQUAL,
    opcodes.VE_SET_NOT_EQUAL,

    opcodes.MATH_NO_OP,
    opcodes.ME_EXP_BASE2_DX,
    opcodes.ME_LOG_BASE2_DX,
    opcodes.ME_EXP_BASEE_FF,
    opcodes.ME_LIGHT_COEFF_DX,
    opcodes.ME_POWER_FUNC_FF,
    opcodes.ME_RECIP_DX,
    opcodes.ME_RECIP_FF,
    opcodes.ME_RECIP_SQRT_DX,
    opcodes.ME_RECIP_SQRT_FF,
    opcodes.ME_MULTIPLY,
    opcodes.ME_EXP_BASE2_FULL_DX,
    opcodes.ME_LOG_BASE2_FULL_DX,
    opcodes.ME_POWER_FUNC_FF_CLAMP_B,
    opcodes.ME_POWER_FUNC_FF_CLAMP_B1,
    opcodes.ME_POWER_FUNC_FF_CLAMP_01,
    opcodes.ME_SIN,
    opcodes.ME_COS,
    opcodes.ME_LOG_BASE2_IEEE,
    opcodes.ME_RECIP_IEEE,
    opcodes.ME_RECIP_SQRT_IEEE,
    opcodes.ME_PRED_SET_EQ,
    opcodes.ME_PRED_SET_GT,
    opcodes.ME_PRED_SET_GTE,
    opcodes.ME_PRED_SET_NEQ,
    opcodes.ME_PRED_SET_CLR,
    opcodes.ME_PRED_SET_INV,
    opcodes.ME_PRED_SET_POP,
    opcodes.ME_PRED_SET_RESTORE,
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
    saturation = auto()

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
    (KW.saturation    , b"saturation"    , b"sat"),
]

def find_keyword(b: memoryview):
    b = bytes(b)
    for op in operations:
        if op.name == b.upper() or (op.synonym is not None and op.synonym == b.upper()):
            return op
    for keyword, name, synonym in keywords:
        if name == b.lower() or (synonym is not None and synonym == b.lower()):
            return keyword
    return None
