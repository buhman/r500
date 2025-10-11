import pvs_src
import pvs_dst
import pvs_dst_bits
from pprint import pprint

code = [
    0xf00203,
    0xd10001,
    0x1248001,
    0x1248001
]

def out(level, *args):
    sys.stdout.write("    " * level + " ".join(args))

def parse_code(code):

    ix = 0

    pvs_dst_maxlen = max(len(k) for k, _ in  pvs_dst.table)
    print(pvs_dst_maxlen)

    while ix < len(code):
        print(f"ix: {ix // 4}")
        dst_op = code[ix + 0]
        src_op0 = code[ix + 1]
        src_op1 = code[ix + 2]
        src_op2 = code[ix + 3]
        print(f"  dst: {dst_op:08x}")
        for name, parse in pvs_dst.table:
            namepad = name.ljust(pvs_dst_maxlen + 1)
            value = parse(dst_op)
            if name == "OPCODE":
                if pvs_dst.MATH_INST(dst_op):
                    print("   ", namepad, pvs_dst_bits.MATH_OPCODE[value])
                else:
                    print("   ", namepad, pvs_dst_bits.VECTOR_OPCODE[value])
            elif name == "REG_TYPE":
                print("   ", namepad, pvs_dst_bits.PVS_DST_REG[value])
            else:
                print("   ", namepad, value)

        print(f"  src0: {src_op0:08x}")
        for name, parse in pvs_src.table:
            print("   ", name, parse(src_op0))
        print(f"  src1: {src_op1:08x}")
        for name, parse in pvs_src.table:
            print("   ", name, parse(src_op1))
        print(f"  src2: {src_op2:08x}")
        for name, parse in pvs_src.table:
            print("   ", name, parse(src_op2))

        ix += 4

parse_code(code)
