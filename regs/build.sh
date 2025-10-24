set -eux

python parse_pvs.py PVS_DST pvs_bits/pvs_opcode_and_destination_operand.txt > pvs_dst.py
python parse_pvs.py PVS_SRC pvs_bits/pvs_source_operand.txt > pvs_src.py
python parse_pvs.py PVS pvs_bits/pvs_dual_math_instruction.txt > pvs_dual_math.py
python parse_pvs_bits.py pvs_bits/pvs_opcode_and_destination_operand_bits.txt > pvs_dst_bits.py
python parse_pvs_bits.py pvs_bits/pvs_source_operand_bits.txt > pvs_src_bits.py
python 3d_registers.py python 3d_registers.txt > registers_lookup.py
