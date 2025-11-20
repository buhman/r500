import sys
from textwrap import indent

from registers_lookup import registers_lookup
from decode_bits import decode_bits

with open(sys.argv[1]) as f:
    values = [
        int(s.strip(), 16)
        for s in f.read().strip().split(",")
        if s
    ]

undocumented_registers = {
    0x1720: "WAIT_UNTIL",
    0x2184: "VAP_VSM_VTX_ASSM",
}

def decode_print(register_name, value, paren=False, display_register_name=None):
    if display_register_name is None:
        display_register_name = register_name
    decoded_value = decode_bits(register_name, value)
    head = decoded_value[0][2:]
    tail = indent('\n'.join([f"= {head}", *decoded_value[1:]]), '      ')
    if paren:
        print(f"  ({display_register_name})\n{tail}")
    else:
        print(f"  {display_register_name}\n{tail}")

class Parser:
    def __init__(self, values):
        self.ix = 0
        self.values = values

    def peek(self):
        return self.values[self.ix]

    def consume(self):
        value = self.peek()
        self.ix += 1
        return value

    def skip(self):
        self.ix += 1

    def packet_type_0(self):
        header = self.consume()
        count = (header >> 16) & 0x3fff
        one_reg = (header >> 15) & 0b1
        reserved = (header >> 13) & 0b11
        base_index = (header >> 0) & 0x1fff
        assert reserved == 0, header
        if one_reg:
            print(f"type 0: {base_index:04x} {count} ONE_REG")
        else:
            #print(f"type 0: {base_index:04x} {count}")
            pass
        while count >= 0:
            address = base_index << 2
            value = self.consume()
            #print(f"  {address:04x} = {value:08x}")
            if address in registers_lookup:
                register_name = registers_lookup[address]
                try:
                    if one_reg or value == 0:
                        assert False
                    decode_print(register_name, value)
                except AssertionError:
                    print(f"  {register_name} = 0x{value:08x}")
            else:
                print(f"  {undocumented_registers[address]} = 0x{value:08x}")
            count -= 1
            if not one_reg:
                base_index += 1

    def packet_type_1(self):
        header = self.consume()
        assert False, header

    def packet_type_2(self):
        print("type 2")
        self.skip()

    def packet_type_3(self):
        header = self.consume()
        reserved = (header >> 0) & 0xff
        assert reserved == 0, header
        it_opcode = (header >> 8) & 0xff
        count = (header >> 16) & 0x3fff

        opcode_names = dict((v, k) for k, v in [
            ("3D_DRAW_VBUF", 0x28),
            ("3D_DRAW_IMMD", 0x29),
            ("3D_DRAW_INDX", 0x2A),
            ("LOAD_PALETTE", 0x2C),
            ("3D_LOAD_VBPNTR", 0x2F),
            ("INDX_BUFFER", 0x33),
            ("3D_DRAW_VBUF_2", 0x34),
            ("3D_DRAW_IMMD_2", 0x35),
            ("3D_DRAW_INDX_2", 0x36),
            ("3D_CLEAR_HIZ", 0x37),
            ("3D_DRAW_128", 0x39),
        ])
        opcode_name = f"{it_opcode:02x}" if it_opcode not in opcode_names else opcode_names[it_opcode]

        registers = {
            "3D_DRAW_VBUF": ["VAP_VTX_FMT", "VAP_VF_CNTL"],
            "3D_DRAW_IMMD": ["VAP_VTX_FMT", "VAP_VF_CNTL"],
            "3D_DRAW_INDX": ["VAP_VTX_FMT", "VAP_VF_CNTL"],
            "3D_LOAD_VBPNTR": ["VAP_VTX_NUM_ARRAYS",
                               "VAP_VTX_AOS_ATTR01",
                               "VAP_VTX_AOS_ADDR0",
                               "VAP_VTX_AOS_ADDR1",
                               "VAP_VTX_AOS_ATTR23",
                               "VAP_VTX_AOS_ADDR2",
                               "VAP_VTX_AOS_ADDR3",
                               "VAP_VTX_AOS_ATTR45",
                               "VAP_VTX_AOS_ADDR4",
                               "VAP_VTX_AOS_ADDR5",
                               "VAP_VTX_AOS_ATTR67",
                               "VAP_VTX_AOS_ADDR6",
                               "VAP_VTX_AOS_ADDR7",
                               "VAP_VTX_AOS_ATTR89",
                               "VAP_VTX_AOS_ADDR8",
                               "VAP_VTX_AOS_ADDR9",
                               "VAP_VTX_AOS_ATTR1011",
                               "VAP_VTX_AOS_ADDR10",
                               "VAP_VTX_AOS_ADDR11",
                               "VAP_VTX_AOS_ATTR1213",
                               "VAP_VTX_AOS_ADDR12",
                               "VAP_VTX_AOS_ADDR13",
                               "VAP_VTX_AOS_ATTR1415",
                               "VAP_VTX_AOS_ADDR14",
                               "VAP_VTX_AOS_ADDR15"],
            "INDX_BUFFER": [(("ONE_REG_WR", (31, 31)), ("SKIP_COUNT", (18, 16)), ("DESTINATION", (12, 0))),
                            (("BUFFER_BASE", (31, 0)),),
                            (("BUFFER_SIZE", (31, 0)),)],
            "3D_DRAW_VBUF_2": ["VAP_VF_CNTL"],
            "3D_DRAW_IMMD_2": ["VAP_VF_CNTL"],
            "3D_DRAW_INDX_2": ["VAP_VF_CNTL"],
        }

        print(f"type 3: op:{opcode_name} count:{count:04x}")
        ix = 0
        while count >= 0:
            value = self.consume()
            if opcode_name in registers and ix < len(registers[opcode_name]):
                register_name = registers[opcode_name][ix]
                if type(register_name) is str:
                    if "_AOS_ATTR" in register_name:
                        decode_print(register_name[:-2], value, paren=True, display_register_name=register_name)
                    elif "_AOS_ADDR" in register_name:
                        decode_print(register_name[:-1], value, paren=True, display_register_name=register_name)
                    else:
                        decode_print(register_name, value, paren=True)
                else:
                    print(f"  ({opcode_name}__{ix})")
                    for i, desc in enumerate(register_name):
                        eq_bar = '=' if i == 0 else '|'
                        d_name, (high, low) = desc
                        mask = (1 << ((high - low) + 1)) - 1
                        v = (value >> low) & mask
                        print(f'      {eq_bar} {opcode_name}__{ix}__{d_name}({v})')
            else:
                print(f"    {value:08x}")
            count -= 1
            ix += 1

    def packet(self):
        value = self.peek()
        packet_type = (value >> 30) & 0b11
        if packet_type == 0:
            self.packet_type_0()
        elif packet_type == 1:
            self.packet_type_1()
        elif packet_type == 2:
            self.packet_type_2()
        elif packet_type == 3:
            self.packet_type_3()
        else:
            assert False

parser = Parser(values)
while parser.ix < len(values):
    parser.packet()
