from enum import Enum, auto

class KW(Enum):
    # ops
    CMP = auto()
    CND = auto()
    COS = auto()
    D2A = auto()
    DP = auto()
    DP3 = auto()
    DP4 = auto()
    EX2 = auto()
    FRC = auto()
    LN2 = auto()
    MAD = auto()
    MAX = auto()
    MDH = auto()
    MDV = auto()
    MIN = auto()
    RCP = auto()
    RSQ = auto()
    SIN = auto()
    SOP = auto()

    # source/dest
    OUT = auto()
    TEMP = auto()
    FLOAT = auto()
    CONST = auto()
    SRC0 = auto()
    SRC1 = auto()
    SRC2 = auto()
    SRCP = auto()

    # srcp_op
    NEG2 = auto()
    SUB = auto()
    ADD = auto()
    NEG = auto()

    # modifiers
    TEX_SEM_WAIT = auto()

_string_to_keyword = {
    b"CMP": KW.CMP,
    b"CND": KW.CND,
    b"COS": KW.COS,
    b"D2A": KW.D2A,
    b"DP": KW.DP,
    b"DP3": KW.DP3,
    b"DP4": KW.DP4,
    b"EX2": KW.EX2,
    b"FRC": KW.FRC,
    b"LN2": KW.LN2,
    b"MAD": KW.MAD,
    b"MAX": KW.MAX,
    b"MDH": KW.MDH,
    b"MDV": KW.MDV,
    b"MIN": KW.MIN,
    b"RCP": KW.RCP,
    b"RSQ": KW.RSQ,
    b"SIN": KW.SIN,
    b"SOP": KW.SOP,
    b"OUT": KW.OUT,
    b"TEMP": KW.TEMP,
    b"FLOAT": KW.FLOAT,
    b"CONST": KW.CONST,
    b"SRC0": KW.SRC0,
    b"SRC1": KW.SRC1,
    b"SRC2": KW.SRC2,
    b"SRCP": KW.SRCP,
    b"NEG2": KW.NEG2,
    b"SUB": KW.SUB,
    b"ADD": KW.ADD,
    b"NEG": KW.NEG,
    b"TEX_SEM_WAIT": KW.TEX_SEM_WAIT,
}
_keyword_to_string = {v:k for k,v in _string_to_keyword.items()}

def find_keyword(s):
    if s.upper() in _string_to_keyword:
        return _string_to_keyword[s.upper()]
    else:
        return None
