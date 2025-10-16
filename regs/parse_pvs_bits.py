import sys

with open(sys.argv[1]) as f:
    lines = f.read().strip().split('\n')



ix = 0

def next_ix():
    global ix
    while not lines[ix].strip():
        ix += 1
    return ix

while ix < len(lines):
    name = lines[next_ix()]
    assert name.strip().endswith(":")
    name = name.strip().removesuffix(":")
    ix += 1

    print(f"{name} = {{")
    while ix < len(lines) and not lines[next_ix()].strip().endswith(":"):
        key, value = lines[next_ix()].split("=")
        if key.startswith("PVS_DST_REG_"):
            key = key.removeprefix("PVS_DST_REG_")
        print(f'    {value.strip()}: "{key.strip()}",')
        ix += 1
    print("}")
    print(f"{name}_gen = dict((v, k) for k, v in {name}.items())")
    print()
