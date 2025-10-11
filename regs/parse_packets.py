import sys
from registers_lookup import registers_lookup

with open(sys.argv[1]) as f:
    values = [
        int(s.strip(), 16)
        for s in f.read().strip().split(",")
    ]

undocumented_registers = {
    0x1720: "RADEON_WAIT_UNTIL",
    0x2184: "VAP_VSM_VTX_ASSM",
}

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
            print(f"type 0: {base_index:04x} {count}")
        while count >= 0:
            address = base_index << 2
            value = self.consume()
            #print(f"  {address:04x} = {value:08x}")
            if address in registers_lookup:
                print(f"  {registers_lookup[address]} = {value:08x}")
            else:
                print(f"  {undocumented_registers[address]} = {value:08x}")
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

        print(f"type 3: op:{it_opcode:02x} count:{count:04x}")
        while count >= 0:
            value = self.consume()
            print(f"  {value:08x}")
            count -= 1

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
