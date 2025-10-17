set -eux

rm -f 3d_registers_bits.h

echo "#pragma once" > 3d_registers_bits.h
echo >> 3d_registers_bits.h

for filename in bits/*; do
    python generate_bits_python.py $filename >> 3d_registers_bits.h
done

mv 3d_registers_bits.h ../drm
