from os import path
import sys

from parse_bits import parse_file_fields
from parse_bits import aggregate

def mask_from_bits(bits):
    if type(bits) is int:
        return 1
    else:
        high, low = bits
        assert high > low, bits
        length = (high - low) + 1
        return (1 << length) - 1

def low_from_bits(bits):
    if type(bits) is int:
        return bits
    else:
        high, low = bits
        return low

def render_descriptor(prefix, d):
    mask = mask_from_bits(d.bits)
    low = low_from_bits(d.bits)
    print(f"#define {prefix}__{d.field_name}(n) (((n) & {hex(mask)}) << {low})")

def prefix_from_filename(filename):
    prefix = sys.argv[1].removesuffix('.txt')
    prefix = path.split(prefix)[1].upper()
    return prefix

if __name__ == "__main__":
    assert sys.argv[1].endswith('.txt')
    l = list(parse_file_fields(sys.argv[1]))
    prefix = prefix_from_filename(sys.argv[2])
    for descriptor in aggregate(l):
        render_descriptor(prefix, descriptor)
