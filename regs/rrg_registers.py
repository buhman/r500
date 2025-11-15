import sys

with open(sys.argv[1]) as f:
    buf = f.read()

prefixes = sys.argv[2:]
assert len(prefixes) >= 1

lines = [line.strip() for line in buf.strip().split()]

assert len(lines) % 3 == 0

def parse(lines):
    for i in range(len(lines) // 3):
        name = lines[i * 3 + 0]
        address = lines[i * 3 + 1]
        page = lines[i * 3 + 2]

        assert '-' in page, page
        orig_address = address
        for prefix in prefixes:
            if address.startswith(f"{prefix}:"):
                address = address.removeprefix(f"{prefix}:")
        assert address != orig_address
        assert address.startswith("0x")
        address = address.removeprefix("0x")
        address = int(address, 16)
        yield name, address, page


for name, address, page in parse(lines):
    print("{")
    print(f"  .name = \"{name}\",")
    print(f"  .address = {hex(address)},")
    print("},")

#print(f"#define {name} {hex(address)}")
