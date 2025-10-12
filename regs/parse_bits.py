import sys
from typing import Union
import re
from dataclasses import dataclass
from pprint import pprint
from collections import OrderedDict

def split_line_fields(line):
    fields = [0, 17, 24, 32]
    a = line[fields[0]:fields[1]]
    b = line[fields[1]:fields[2]]
    c = line[fields[2]:fields[3]]
    d = line[fields[3]:]
    return a, b, c, d

def parse_file_fields(filename):
    with open(filename) as f:
        lines = f.read().split('\n')
    first, *rest = lines
    a, b, c, d = split_line_fields(first)
    assert a == 'Field Name       ', a
    assert b == 'Bits   ', b
    assert c == 'Default ', c
    assert d.rstrip() == 'Description', d

    for line in rest:
        a, b, c, d = split_line_fields(line)
        yield a.strip(), b.strip(), c.strip(), d.strip()

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

@dataclass
class Descriptor:
    field_name: str
    bits: Union[int, tuple[int, int]]
    default: int
    description: str
    possible_values: list[tuple[int, str]]

def aggregate(fields):
    ix = 0

    descriptor = None

    def next_nonempty():
        nonlocal ix
        while ix < len(fields) and not ''.join(fields[ix]):
            ix += 1

    def parse_possible_values():
        nonlocal ix
        if ix + 1 >= len(fields):
            return
        if not fields[ix+1][0] == '':
            return
        if not fields[ix+1][3] == 'POSSIBLE VALUES:':
            return
        ix += 1
        while ix + 1 < len(fields) and fields[ix+1][0] == '' and fields[ix+1][3] != '':
            field_name, bits, default, description = fields[ix+1]
            assert not field_name, field_name
            assert not bits, bits
            assert not default, default
            assert re.match('^[0-9]{2} - ', description), repr(description)
            yield description
            ix += 1

    def parse_description_lines():
        nonlocal ix
        while ix + 1 < len(fields) and fields[ix+1][0] == '':
            field_name, bits, default, description = fields[ix+1]
            assert not field_name, field_name
            assert not bits, bits
            assert not default, default
            assert not re.match('^[0-9]{2} - ', description), description
            if description == 'POSSIBLE VALUES:':
                break
            else:
                yield description
                ix += 1

    def parse_possible_value_num(s):
        num, description = s.split(' - ')
        num = int(num, 10)
        if ": " in description:
            name, description = description.split(": ")
        else:
            name = None
        return num, (name, description)

    while ix < len(fields):
        field_name, bits, default, description = fields[ix]
        description_lines = [description]
        description_lines.extend(parse_description_lines())
        possible_values = OrderedDict(
            map(parse_possible_value_num, parse_possible_values())
        )

        assert default.startswith('0x'), default
        yield Descriptor(
            field_name = field_name,
            bits = parse_bits(bits),
            default = int(default, 16),
            description = ' '.join(description_lines),
            possible_values = possible_values
        )

        ix += 1
        next_nonempty()

if __name__ == "__main__":
    l = list(parse_file_fields(sys.argv[1]))
    for descriptor in aggregate(l):
        pprint(descriptor, width=200)
