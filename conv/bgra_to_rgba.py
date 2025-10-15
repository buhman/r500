import sys

with open(sys.argv[1], 'rb') as f:
    buf = f.read()

out = bytearray(len(buf))

for i in range(len(buf) // 4):
    b = buf[i * 4 + 0]
    g = buf[i * 4 + 1]
    r = buf[i * 4 + 2]
    a = 255

    out[i * 4 + 0] = r
    out[i * 4 + 1] = g
    out[i * 4 + 2] = b
    out[i * 4 + 3] = a

with open(sys.argv[2], 'wb') as f:
    f.write(out)
