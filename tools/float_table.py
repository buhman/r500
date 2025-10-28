nums = """
        0 	0.001953125 	0.00390625 	0.005859375 	0.0078125 	0.009765625 	0.01171875 	0.013671875
 	0.015625 	0.017578125 	0.01953125 	0.021484375 	0.0234375 	0.025390625 	0.02734375 	0.029296875
 	0.03125 	0.03515625 	0.0390625 	0.04296875 	0.046875 	0.05078125 	0.0546875 	0.05859375
 	0.0625 	0.0703125 	0.078125 	0.0859375 	0.09375 	0.1015625 	0.109375 	0.1171875
 	0.125 	0.140625 	0.15625 	0.171875 	0.1875 	0.203125 	0.21875 	0.234375
 	0.25 	0.28125 	0.3125 	0.34375 	0.375 	0.40625 	0.4375 	0.46875
 	0.5 	0.5625 	0.625 	0.6875 	0.75 	0.8125 	0.875 	0.9375
 	1 	1.125 	1.25 	1.375 	1.5 	1.625 	1.75 	1.875
 	2 	2.25 	2.5 	2.75 	3 	3.25 	3.5 	3.75
 	4 	4.5 	5 	5.5 	6 	6.5 	7 	7.5
 	8 	9 	10 	11 	12 	13 	14 	15
 	16 	18 	20 	22 	24 	26 	28 	30
 	32 	36 	40 	44 	48 	52 	56 	60
 	64 	72 	80 	88 	96 	104 	112 	120
 	128 	144 	160 	176 	192 	208 	224 	240
"""

nums = [float(i) for i in nums.split()]

def parse_043_float(i):
    get_significand = lambda i: (i >> 0) & 0b111
    get_exponent = lambda i: (i >> 3) & 0b1111
    # ieee_754_bias = 2 ** (4 - 1) - 1
    bias = 7

    exponent = get_exponent(i)
    significand = get_significand(i)

    base = 8
    if exponent > 0:
        significand |= 0b1000
    else:
        # denormal
        if significand == 0:
            significand = 0b0001
            base = 16
        exponent = 1

    value = (significand / base) * 2 ** (exponent - bias)
    return value

for i, num in enumerate(nums):
    if num == 0:
        continue
    assert num == parse_043_float(i)

assert max(range(128)) == 127
assert min(range(128)) == 0

def find_nearest(n):
    value = parse_043_float(0)
    nearest = 0
    nearest_diff = abs(value - n)

    for i in range(1, 128):
        value = parse_043_float(i)
        diff = abs(value - n)
        if diff < nearest_diff:
            nearest = i
            nearest_diff = diff

    return nearest, parse_043_float(nearest)

if __name__ == "__main__":
    import sys
    number = sys.argv[1]
    if '.' in number:
        number = float(number)
        print("nearest", number, *find_nearest(number))
    else:
        number = int(number, 10)
        print(parse_043_float(number))
