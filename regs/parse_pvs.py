import sys
from typing import Union
import re
from dataclasses import dataclass
from pprint import pprint

def split_line_fields(line):
    fields = [0, 22, 30]
    a = line[fields[0]:fields[1]]
    b = line[fields[1]:fields[2]]
    c = line[fields[2]:]
    return a, b, c

def parse_file_fields(filename):
    with open(filename) as f:
        lines = f.read().split('\n')
    first, *rest = lines
    a, b, c = split_line_fields(first)
    assert a.rstrip() == 'Field Name', a
    assert b.rstrip() == 'Bits', b
    assert c.rstrip() == 'Description', c

    for line in rest:
        if not line.strip():
            continue
        a, b, c = split_line_fields(line)
        yield a.strip(), b.strip(), c.strip()

def parse_bits(s):
    if ':' in s:
        a, b = s.split(':')
        h = int(a, 10)
        l = int(b, 10)
        assert h > l
        return h, l
    else:
        b = int(s, 10)
        return b

def parse_file_fields2(filename):
    for line in parse_file_fields(filename):
        field_name, bits, description = line
        bits = parse_bits(bits)
        if not field_name.startswith(prefix + '_'):
            assert field_name.startswith("SPARE_")
            continue
        field_name = field_name.removeprefix(prefix + '_')
        yield field_name, bits, description

def out(level, *args):
    sys.stdout.write("    " * level + " ".join(args) + '\n')

def mask_from_bits(bits):
    if type(bits) is tuple:
        high, low = bits
        assert high > low, (high, low)
    else:
        high = bits
        low = bits
    length = (high - low) + 1
    return (1 << length) - 1

def low_from_bits(bits):
    if type(bits) is tuple:
        return bits[1]
    else:
        return bits

def generate_python(prefix, fields):
    #out(0, f"class {prefix}:")
    fields = list(fields)
    for field_name, bits, description in fields:
        #out(1, f"@staticmethod")
        out(0, f"def {field_name}(n):")
        out(1, f"return (n >> {low_from_bits(bits)}) & {mask_from_bits(bits)}")
        out(0, "")

    out(0, "table = [")
    for field_name, bits, description in fields:
        out(1, f'("{field_name}", {field_name}),')
    out(0, "]")

prefix = sys.argv[1]
filename = sys.argv[2]
generate_python(prefix, parse_file_fields2(filename))
