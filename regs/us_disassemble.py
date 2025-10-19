import sys
import parse_bits
from collections import OrderedDict
from os import path

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
        filename = path.join("bits", register.lower() + ".txt")
        l = list(parse_bits.parse_file_fields(filename))
        yield register, OrderedDict(
            (d.field_name, d) for d in parse_bits.aggregate(l)
        )

registers = dict(parse_registers())
US_CMN_INST = registers["US_CMN_INST"]

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

    def inner2(i, register_name):
        max_length = max(map(len, registers[register_name])) + 1

        value = code[ix + i]
        yield f"{register_name}"
        yield f"  [{value:08x}]"
        if register_name == 0:
            assert value == 0
            return
        register = registers[register_name]
        for d in register.values():
            field_pv_name = get_field_pv_name(value, d)
            yield ' '.join([d.field_name.ljust(max_length), f"{field_pv_name}"])

    def inner(register_name_list):

        columns = []
        for i, register_name in enumerate(register_name_list):
            columns.append(list(inner2(i, register_name)))
        column_widths = [35, 26, 24, 24, 27, 22]
        column_height = max(len(column) for column in columns)
        assert len(columns) == 6

        def get_row(column, rix, cix):
            if rix < len(column):
                value = column[rix]
            else:
                value = ''
            return value.ljust(column_widths[cix]) + '| '

        for rix in range(column_height):
            row = ''.join([
                get_row(column, rix, cix)
                for cix, column in enumerate(columns)
            ])
            print("  ", row)

    inst_type = get_field_pv_name(us_cmn_inst, US_CMN_INST["TYPE"])
    if inst_type in {"US_INST_TYPE_OUT", "US_INST_TYPE_ALU"}:
        inner(alu_output)
    elif inst_type == "US_INST_TYPE_FC":
        inner(fc)
    elif inst_type == "US_INST_TYPE_TEX":
        inner(tex)
    else:
        assert False, inst_type

def parse_hex(s):
    assert s.startswith('0x')
    return int(s.removeprefix('0x'), 16)

if __name__ == "__main__":
    filename = sys.argv[1]
    with open(filename) as f:
        buf = f.read()
    code = [parse_hex(c.strip()) for c in buf.split(',') if c.strip()]
    for i in range(len(code) // 6):

        disassemble(code, i * 6)
