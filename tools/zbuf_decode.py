import sys
import struct

with open(sys.argv[1], 'rb') as f:
    buf = memoryview(f.read())

print("P2")
print("1600 1200")
print("65535")

min_n = None
max_n = None

for i in range(len(buf) // 4):
    b = buf[i*4:i*4+4]

    num, = struct.unpack("<I", b)
    assert (num & 0xff) == 0, (hex(i*4), hex(num))
    depth24 = num >> 8
    depth16 = num >> 16

    if i == 0:
        min_n = depth24
        max_n = depth24

    if min_n == 0 or (depth24 < min_n and depth24 != 0):
        min_n = depth24
    if depth24 > max_n:
        max_n = depth24

    print(depth16, end='')
    if i % 128 == 127:
        print("", end='\n')
    else:
        print("", end=" ")


print(min_n, max_n, file=sys.stderr)
