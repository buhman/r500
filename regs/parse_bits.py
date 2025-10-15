import sys
from typing import Union
import re
from dataclasses import dataclass
from pprint import pprint
from collections import OrderedDict

def split_line_fields(line, fields):
    a = line[fields[0]:fields[1]]
    b = line[fields[1]:fields[2]]
    c = line[fields[2]:fields[3]]
    d = line[fields[3]:]
    assert a.strip() == '' or a[0] != ' ', a
    assert b.strip() == '' or b[0] != ' ', b
    assert c.strip() == '' or c[0] != ' ', c
    assert d.strip() == '' or d[0] != ' ', d
    assert a[-1] == ' '
    assert b[-1] == ' '
    assert c[-1] == ' ' or len(line) < fields[3]
    return a, b, c, d

def find_line_fields(line):
    field_name_ix = line.index('Field Name')
    bits_ix = line.index('Bits')
    default_ix = line.index('Default')
    description_ix = line.index('Description')
    assert field_name_ix == 0
    assert bits_ix > field_name_ix
    assert default_ix > bits_ix
    assert description_ix > default_ix
    return field_name_ix, bits_ix, default_ix, description_ix

def parse_file_fields(filename):
    with open(filename) as f:
        lines = f.read().split('\n')
    first, *rest = lines
    fields = find_line_fields(first)
    a, b, c, d = split_line_fields(first, fields)
    assert a.rstrip() == 'Field Name', a
    assert b.rstrip() == 'Bits', b
    assert c.rstrip() == 'Default', c
    assert d.rstrip() == 'Description', d

    for line in rest:
        if not line.strip():
            continue
        a, b, c, d = split_line_fields(line, fields)
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
        if not fields[ix+1][3] == 'POSSIBLE VALUES:':
            return
        ix += 1

        while ix + 1 < len(fields) and fields[ix+1][0] == '' and fields[ix+1][3] != '':
            field_name, bits, default, description = fields[ix+1]
            assert not field_name, field_name
            assert not bits, bits
            assert not default, default
            m = re.match('^([0-9]{2}) - (.*)$', description)
            assert m, repr(description)
            value, desc = m.groups()
            if ": " in desc and ' ' not in desc.split(": ")[0].strip():
                name, desc = desc.split(": ")
                name = name
            elif len(desc.strip().split()) == 1:
                name = desc
                desc = None
            else:
                name = None

            if name is not None:
                name = name.strip().upper().replace('.', '_').replace('-', '_').replace(',', '_').replace('*', 'x').replace('+', 'p')

            if name != 'RESERVED':
                yield int(value, 10), (name, desc)

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

    while ix < len(fields):
        field_name, bits, default, description = fields[ix]
        if description == 'POSSIBLE VALUES:':
            description_lines = []
            ix -= 1
        else:
            description_lines = [description]
            description_lines.extend(parse_description_lines())
        possible_values = OrderedDict(parse_possible_values())

        assert default.startswith('0x') or default == 'none', default
        yield Descriptor(
            field_name = field_name,
            bits = parse_bits(bits),
            default = 0 if default == 'none' else int(default, 16),
            description = ' '.join(description_lines),
            possible_values = possible_values
        )

        ix += 1
        next_nonempty()

if __name__ == "__main__":
    l = list(parse_file_fields(sys.argv[1]))
    for descriptor in aggregate(l):
        pprint(descriptor, width=200)
