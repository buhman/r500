import parse_bits
from pprint import pprint
from collections import OrderedDict

register_names = [
    "US_CMN_INST",
    "US_ALU_RGB_ADDR",
    "US_ALU_ALPHA_ADDR",
    "US_ALU_RGB_INST",
    "US_ALU_ALPHA_INST",
    "US_ALU_RGBA_INST",
    "US_TEX_INST",
    "US_TEX_ADDR",
    "US_TEX_ADDR_DXDY",
    "US_FC_INST",
    "US_FC_ADDR",
]

alu_output = [
    "US_CMN_INST",
    "US_ALU_RGB_ADDR",
    "US_ALU_ALPHA_ADDR",
    "US_ALU_RGB_INST",
    "US_ALU_ALPHA_INST",
    "US_ALU_RGBA_INST",
]

tex = [
    "US_CMN_INST",
    "US_TEX_INST",
    "US_TEX_ADDR",
    "US_TEX_ADDR_DXDY",
    0,
    0,
]

fc = [
    "US_CMN_INST",
    0,
    "US_FC_INST",
    "US_FC_ADDR",
    0,
    0,
]

def parse_registers():
    for register in register_names:
        filename = register.lower() + ".txt"
        l = list(parse_bits.parse_file_fields(filename))
        yield register, OrderedDict(
            (d.field_name, d) for d in parse_bits.aggregate(l)
        )

registers = dict(parse_registers())
US_CMN_INST = registers["US_CMN_INST"]

"""
code = [
    0x00078005,
    0x08020080,
    0x08020080,
    0x1c9b04d8,
    0x1c810003,
    0x00000005,
]
"""

# DCL IN[0].xyz, GENERIC[0], PERSPECTIVE
# DCL OUT[0], COLOR
# IMM[0] FLT32 {    1.0000,     0.0000,     0.0000,     0.0000}
#   0: MOV OUT[0].xyz, IN[0].xyzx
#   1: MOV OUT[0].w, IMM[0].xxxx
#   2: END
# Radeon Compiler Program
#  0: src0.xyz = input[0]
#     MAX color[0].xyz (OMOD DISABLE), src0.xyz, src0.xyz
#     MAX color[0].w (OMOD DISABLE), src0.1, src0.1
code = [
    0x00078005,
    0x08020000,
    0x08020080,
    0x1c440220,
    0x1cc18003,
    0x00000005,
]

def get_field(n, descriptor):
    if type(descriptor.bits) is int:
        return (n >> descriptor.bits) & 1
    else:
        high, low = descriptor.bits
        assert high > low
        mask_length = (high - low) + 1
        mask = (1 << mask_length) - 1
        return (n >> low) & mask

def get_possible_value(value, descriptor):
    if not descriptor.possible_values:
        return value

    name, description = descriptor.possible_values[value]
    if name is not None:
        return name
    else:
        if len(description) < 20:
            return description
        else:
            return value

def get_field_pv_name(value, descriptor):
    return get_possible_value(get_field(value, descriptor), descriptor)

def disassemble(code, ix):
    us_cmn_inst = code[ix + 0]
    print(f"{ix:04x}")
    max_length = max(map(len, US_CMN_INST.keys())) + 1

    def inner(register_name_list):
        for i, register_name in enumerate(register_name_list):
            value = code[ix + i]
            register = registers[register_name]
            print(" ", f"{value:08x}", register_name)
            for d in register.values():
                field_pv_name = get_field_pv_name(value, d)
                print("   ", d.field_name.ljust(max_length), field_pv_name)

    inst_type = get_field_pv_name(us_cmn_inst, US_CMN_INST["TYPE"])
    if inst_type in {"US_INST_TYPE_OUT", "US_INST_TYPE_ALU"}:
        inner(alu_output)
    elif inst_type == "US_INST_TYPE_FC":
        inner(fc)
    elif inst_type == "US_INST_TYPE_TEX":
        inner(tex)
    else:
        assert False, inst_type

disassemble(code, 0)
