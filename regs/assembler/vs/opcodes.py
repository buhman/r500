from dataclasses import dataclass
from typing import Optional

@dataclass
class MVE:
    name: str
    synonym: Optional[str]
    value: int
    operand_count: int

@dataclass
class VE:
    name: str
    synonym: Optional[str]
    value: int
    operand_count: int

@dataclass
class ME:
    name: str
    synonym: Optional[str]
    value: int
    operand_count: int

MACRO_OP_2CLK_MADD        = MVE(b"MACRO_OP_2CLK_MADD"       , None      , 0,  3)
MACRO_OP_2CLK_M2X_ADD     = MVE(b"MACRO_OP_2CLK_M2X_ADD"    , None      , 1,  3)

VECTOR_NO_OP              = VE(b"VECTOR_NO_OP"              , b"VE_NOP" , 0,  0)
VE_DOT_PRODUCT            = VE(b"VE_DOT_PRODUCT"            , b"VE_DOT" , 1,  2)
VE_MULTIPLY               = VE(b"VE_MULTIPLY"               , b"VE_MUL" , 2,  2)
VE_ADD                    = VE(b"VE_ADD"                    , None      , 3,  2)
VE_MULTIPLY_ADD           = VE(b"VE_MULTIPLY_ADD"           , b"VE_MAD" , 4,  3)
VE_DISTANCE_VECTOR        = VE(b"VE_DISTANCE_VECTOR"        , None      , 5,  2)
VE_FRACTION               = VE(b"VE_FRACTION"               , b"VE_FRC" , 6,  1)
VE_MAXIMUM                = VE(b"VE_MAXIMUM"                , b"VE_MAX" , 7,  2)
VE_MINIMUM                = VE(b"VE_MINIMUM"                , b"VE_MIN" , 8,  2)
VE_SET_GREATER_THAN_EQUAL = VE(b"VE_SET_GREATER_THAN_EQUAL" , b"VE_SGE" , 9,  2)
VE_SET_LESS_THAN          = VE(b"VE_SET_LESS_THAN"          , b"VE_SLT" , 10, 2)
VE_MULTIPLYX2_ADD         = VE(b"VE_MULTIPLYX2_ADD"         , None      , 11, 3)
VE_MULTIPLY_CLAMP         = VE(b"VE_MULTIPLY_CLAMP"         , None      , 12, 3)
VE_FLT2FIX_DX             = VE(b"VE_FLT2FIX_DX"             , None      , 13, 1)
VE_FLT2FIX_DX_RND         = VE(b"VE_FLT2FIX_DX_RND"         , None      , 14, 1)
VE_PRED_SET_EQ_PUSH       = VE(b"VE_PRED_SET_EQ_PUSH"       , None      , 15, 2)
VE_PRED_SET_GT_PUSH       = VE(b"VE_PRED_SET_GT_PUSH"       , None      , 16, 2)
VE_PRED_SET_GTE_PUSH      = VE(b"VE_PRED_SET_GTE_PUSH"      , None      , 17, 2)
VE_PRED_SET_NEQ_PUSH      = VE(b"VE_PRED_SET_NEQ_PUSH"      , None      , 18, 2)
VE_COND_WRITE_EQ          = VE(b"VE_COND_WRITE_EQ"          , None      , 19, 2)
VE_COND_WRITE_GT          = VE(b"VE_COND_WRITE_GT"          , None      , 20, 2)
VE_COND_WRITE_GTE         = VE(b"VE_COND_WRITE_GTE"         , None      , 21, 2)
VE_COND_WRITE_NEQ         = VE(b"VE_COND_WRITE_NEQ"         , None      , 22, 2)
VE_COND_MUX_EQ            = VE(b"VE_COND_MUX_EQ"            , None      , 23, 3)
VE_COND_MUX_GT            = VE(b"VE_COND_MUX_GT"            , None      , 24, 3)
VE_COND_MUX_GTE           = VE(b"VE_COND_MUX_GTE"           , None      , 25, 3)
VE_SET_GREATER_THAN       = VE(b"VE_SET_GREATER_THAN"       , b"VE_SGT" , 26, 2)
VE_SET_EQUAL              = VE(b"VE_SET_EQUAL"              , b"VE_SEQ" , 27, 2)
VE_SET_NOT_EQUAL          = VE(b"VE_SET_NOT_EQUAL"          , b"VE_SNE" , 28, 2)

MATH_NO_OP                = ME(b"MATH_NO_OP"                , b"ME_NOP" , 0,  0)
ME_EXP_BASE2_DX           = ME(b"ME_EXP_BASE2_DX"           , b"ME_EXP" , 1,  1)
ME_LOG_BASE2_DX           = ME(b"ME_LOG_BASE2_DX"           , b"ME_LOG2", 2,  1)
ME_EXP_BASEE_FF           = ME(b"ME_EXP_BASEE_FF"           , b"ME_EXPE", 3,  1)
ME_LIGHT_COEFF_DX         = ME(b"ME_LIGHT_COEFF_DX"         , None      , 4,  3)
ME_POWER_FUNC_FF          = ME(b"ME_POWER_FUNC_FF"          , b"ME_POW" , 5,  2)
ME_RECIP_DX               = ME(b"ME_RECIP_DX"               , b"ME_RCP" , 6,  1)
ME_RECIP_FF               = ME(b"ME_RECIP_FF"               , None      , 7,  1)
ME_RECIP_SQRT_DX          = ME(b"ME_RECIP_SQRT_DX"          , b"ME_RSQ" , 8,  1)
ME_RECIP_SQRT_FF          = ME(b"ME_RECIP_SQRT_FF"          , None      , 9,  1)
ME_MULTIPLY               = ME(b"ME_MULTIPLY"               , b"ME_MUL" , 10, 2)
ME_EXP_BASE2_FULL_DX      = ME(b"ME_EXP_BASE2_FULL_DX"      , None      , 11, 1)
ME_LOG_BASE2_FULL_DX      = ME(b"ME_LOG_BASE2_FULL_DX"      , None      , 12, 1)
ME_POWER_FUNC_FF_CLAMP_B  = ME(b"ME_POWER_FUNC_FF_CLAMP_B"  , None      , 13, 3)
ME_POWER_FUNC_FF_CLAMP_B1 = ME(b"ME_POWER_FUNC_FF_CLAMP_B1" , None      , 14, 3)
ME_POWER_FUNC_FF_CLAMP_01 = ME(b"ME_POWER_FUNC_FF_CLAMP_01" , None      , 15, 2)
ME_SIN                    = ME(b"ME_SIN"                    , None      , 16, 1)
ME_COS                    = ME(b"ME_COS"                    , None      , 17, 1)
ME_LOG_BASE2_IEEE         = ME(b"ME_LOG_BASE2_IEEE"         , None      , 18, 1)
ME_RECIP_IEEE             = ME(b"ME_RECIP_IEEE"             , None      , 19, 1)
ME_RECIP_SQRT_IEEE        = ME(b"ME_RECIP_SQRT_IEEE"        , None      , 20, 1)
ME_PRED_SET_EQ            = ME(b"ME_PRED_SET_EQ"            , None      , 21, 1)
ME_PRED_SET_GT            = ME(b"ME_PRED_SET_GT"            , None      , 22, 1)
ME_PRED_SET_GTE           = ME(b"ME_PRED_SET_GTE"           , None      , 23, 1)
ME_PRED_SET_NEQ           = ME(b"ME_PRED_SET_NEQ"           , None      , 24, 1)
ME_PRED_SET_CLR           = ME(b"ME_PRED_SET_CLR"           , None      , 25, 0)
ME_PRED_SET_INV           = ME(b"ME_PRED_SET_INV"           , None      , 26, 1)
ME_PRED_SET_POP           = ME(b"ME_PRED_SET_POP"           , None      , 27, 1)
ME_PRED_SET_RESTORE       = ME(b"ME_PRED_SET_RESTORE"       , None      , 28, 1)
