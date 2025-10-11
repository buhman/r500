set -eux

python parse_pvs.py PVS_DST pvs_opcode_and_destination_operand.txt > pvs_dst.py
python parse_pvs.py PVS_SRC pvs_source_operand.txt > pvs_src.py
python parse_pvs_bits.py regs/pvs_opcode_and_destination_operand_bits.txt > pvs_dst_bits.py
