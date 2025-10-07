with open('3d_registers.txt') as f:
    lines = f.read().split('\n')

for line in lines:
    if not line.strip():
        continue
    reg_name = line.split(':')[1].split(' ')[0]
    reg_value = line.split('MMReg:')[1]
    print("#define", reg_name, reg_value)
