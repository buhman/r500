from os import path
from functools import partial

import parse_bits

class BaseRegister:
    def set(self, code, value, *, code_ix, descriptor):
        if type(descriptor.bits) is int:
            mask = 1
            low = descriptor.bits
        else:
            high, low = descriptor.bits
            assert high > low
            mask_length = (high - low) + 1
            mask = (1 << mask_length) - 1

        code_value = code[code_ix]
        assert (code_value >> low) & mask == 0
        assert value & mask == value
        code[code_ix] |= (value & mask) << low

_descriptor_indicies = {
    "US_CMN_INST": 0,
    "US_ALU_RGB_ADDR": 1,
    "US_ALU_ALPHA_ADDR": 2,
    "US_ALU_RGB_INST": 3,
    "US_ALU_ALPHA_INST": 4,
    "US_ALU_RGBA_INST": 5,

    "US_TEX_INST": 1,
    "US_TEX_ADDR": 2,
    "US_TEX_ADDR_DXDY": 3,

    "US_FC_INST": 2,
    "US_FC_ADDR": 3,
}

def parse_register(register_name):
    base = path.dirname(__file__)

    filename = path.join(base, "..", "..", "bits", register_name.lower() + ".txt")
    l = list(parse_bits.parse_file_fields(filename))
    cls = type(register_name, (BaseRegister,), {})
    instance = cls()
    descriptors = list(parse_bits.aggregate(l))
    code_ix = _descriptor_indicies[register_name]
    for descriptor in descriptors:
        setattr(instance, descriptor.field_name,
                partial(instance.set, code_ix=code_ix, descriptor=descriptor))
        func = getattr(instance, descriptor.field_name)
        for pv_value, (pv_name, _) in descriptor.possible_values.items():
            if pv_name is not None:
                setattr(func, pv_name, pv_value)
    assert getattr(instance, "descriptors", None) is None
    instance.descriptors = descriptors

    return instance

US_CMN_INST = parse_register("US_CMN_INST")
US_ALU_RGB_ADDR = parse_register("US_ALU_RGB_ADDR")
US_ALU_ALPHA_ADDR = parse_register("US_ALU_ALPHA_ADDR")
US_ALU_RGB_INST = parse_register("US_ALU_RGB_INST")
US_ALU_ALPHA_INST = parse_register("US_ALU_ALPHA_INST")
US_ALU_RGBA_INST = parse_register("US_ALU_RGBA_INST")
US_TEX_INST = parse_register("US_TEX_INST")
US_TEX_ADDR = parse_register("US_TEX_ADDR")
US_TEX_ADDR_DXDY = parse_register("US_TEX_ADDR_DXDY")
US_FC_INST = parse_register("US_FC_INST")
US_FC_ADDR = parse_register("US_FC_ADDR")
