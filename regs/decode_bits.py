import sys
from os import path
import re

from parse_bits import parse_file_fields
from parse_bits import aggregate
from generate_bits_python import mask_from_bits
from generate_bits_python import low_from_bits
from generate_bits_python import prefix_from_filename

import generate_bits_python

def bit_definition_filename(s):
    s = s.lower()
    base = path.dirname(generate_bits_python.__file__)

    try_names = [s]
    m = re.match('^(.+?)([0-9]+)$', s)
    if m:
        group = m.group(1)
        try_names.append(group)
        if group.endswith("_"):
            try_names.append(group.removesuffix("_"))

    for name in try_names:
        pname = f"{name}.txt"
        p = path.join(base, "bits", pname)
        if path.exists(p):
            return p

    assert False, s

def find_name(descriptor, bit_value):
    for value, (name, _) in descriptor.possible_values.items():
        if value == bit_value and name is not None:
            return f"{descriptor.field_name}__{name} // {bit_value}"
    return f"{descriptor.field_name}({bit_value})"

def decode_bits(reg_name, value):
    filename = bit_definition_filename(reg_name)
    l = list(parse_file_fields(filename))
    prefix = prefix_from_filename(filename)
    orig_value = value
    gen_value = 0

    lines = []

    for i, descriptor in enumerate(aggregate(l)):
        mask = mask_from_bits(descriptor.bits)
        low = low_from_bits(descriptor.bits)
        bit_value = (value >> low) & mask
        dot = ',' if i == 0 else '|'

        lines.append(f"{dot} {prefix}__{find_name(descriptor, bit_value)}")
        value &= ~(mask << low)
        gen_value |= (bit_value << low)
    assert value == 0, (hex(value), hex(orig_value))
    assert orig_value == gen_value

    return lines

if __name__ == "__main__":
    reg_name = sys.argv[1]
    value_str = sys.argv[2]
    value = int(value_str, 16)

    print("\n".join(decode_bits(reg_name, value)))
