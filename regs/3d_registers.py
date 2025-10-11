with open('3d_registers.txt') as f:
    lines = f.read().split('\n')

def parse_reg_value(value):
    if '-' in value:
        start, end = value.split('-')
        return int(start, 16), int(end, 16)
    else:
        return int(value, 16)

def reg_name_value(lines):
    for line in lines:
        if not line.strip():
            continue
        reg_name = line.split(':')[1].split(' ')[0]
        reg_value = line.split(' MMReg:')[1:]
        for value in reg_value:
            yield reg_name, parse_reg_value(value.removesuffix(','))

#for reg_name, reg_value in reg_name_value(lines):
#    print("#define", reg_name, reg_value)

aos_order_table = [
    "VTX_AOS_ATTR01",
    "VTX_AOS_ADDR0",
    "VTX_AOS_ADDR1",
    "VTX_AOS_ATTR23",
    "VTX_AOS_ADDR2",
    "VTX_AOS_ADDR3",
    "VTX_AOS_ATTR45",
    "VTX_AOS_ADDR4",
    "VTX_AOS_ADDR5",
    "VTX_AOS_ATTR67",
    "VTX_AOS_ADDR6",
    "VTX_AOS_ADDR7",
    "VTX_AOS_ATTR89",
    "VTX_AOS_ADDR8",
    "VTX_AOS_ADDR8",
    "VTX_AOS_ATTR1011",
    "VTX_AOS_ADDR10",
    "VTX_AOS_ADDR11",
    "VTX_AOS_ATTR1213",
    "VTX_AOS_ADDR12",
    "VTX_AOS_ADDR13",
    "VTX_AOS_ATTR1415",
    "VTX_AOS_ADDR14",
    "VTX_AOS_ADDR15",
]

print("registers_lookup = {")
for reg_name, reg_value in reg_name_value(lines):
    if type(reg_value) is int:
        print(f'    0x{reg_value:04x}: "{reg_name}",')
    else:
        start, end = reg_value
        if '[' not in reg_name:
            offset = start
            while offset <= end:
                ix = (offset - start) // 4
                print(f'    0x{offset:04x}: "{reg_name}[{ix}]",')
                offset += 4
        else:
            reg_basename = reg_name.split('[')[0]
            index_range = reg_name.split('[')[1].split(']')[0]
            start_ix, end_ix = map(int, index_range.split('-'))

            offset_increment = 4

            if (end - start) // 4 != end_ix - start_ix:
                # guess the offset increment
                if start + end_ix * 8 == end:
                    offset_increment = 8
                elif start + end_ix * 16 == end:
                    offset_increment = 16
                elif reg_basename == 'VAP_VTX_AOS_ADDR':
                    pass
                elif reg_basename == 'VAP_VTX_AOS_ATTR':
                    for i, name in enumerate(aos_order_table):
                        print(f'    0x{i * 4 + start}: "{name}",')
                    continue
                else:
                    print(reg_name, reg_value)
                    assert False

            offset = start
            while offset <= end:
                ix = (offset - start) // offset_increment
                print(f'    0x{offset:04x}: "{reg_basename}{ix}",')
                offset += offset_increment

print('}')
