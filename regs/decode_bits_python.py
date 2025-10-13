import sys

from parse_bits import parse_file_fields
from parse_bits import aggregate
from generate_bits_python import mask_from_bits
from generate_bits_python import low_from_bits
from generate_bits_python import prefix_from_filename

l = list(parse_file_fields(sys.argv[1]))
prefix = prefix_from_filename(sys.argv[1])
assert sys.argv[2].startswith('0x')
value = int(sys.argv[2], 16)
orig_value = value
gen_value = 0
for i, descriptor in enumerate(aggregate(l)):
    mask = mask_from_bits(descriptor.bits)
    low = low_from_bits(descriptor.bits)
    bit_value = (value >> low) & mask
    dot = ',' if i == 0 else '|'
    print(f"{dot} {prefix}__{descriptor.field_name}({bit_value})")
    value &= ~(mask << low)
    gen_value |= (bit_value << low)

assert value == 0
assert orig_value == gen_value
