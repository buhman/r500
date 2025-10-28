import struct
import sys

with open(sys.argv[1], 'r') as f:
    buf = f.read()

nums = [int(i.strip(), 16) for i in buf.split(',') if i.strip()]

for num in nums:
    data = struct.pack("<I", num)
    f, = struct.unpack("<f", data)
    print(f, 1024 * f)
