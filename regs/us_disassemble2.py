import sys
from os import path, environ
import parse_bits
from collections import OrderedDict
from functools import partial
from pprint import pprint
import struct

VERBOSE = environ.get("VERBOSE", "false").lower() == "true"

class BaseRegister:
    def get(self, code, *, code_ix, descriptor):
        n = code[code_ix]
        if type(descriptor.bits) is int:
            return (n >> descriptor.bits) & 1
        else:
            high, low = descriptor.bits
            assert high > low
            mask_length = (high - low) + 1
            mask = (1 << mask_length) - 1
            return (n >> low) & mask

    def get_name(self, n, *, code_ix, descriptor):
        value = self.get(n, code_ix=code_ix, descriptor=descriptor)
        return value, *descriptor.possible_values[value]

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

    filename = path.join(base, "bits", register_name.lower() + ".txt")
    l = list(parse_bits.parse_file_fields(filename))
    cls = type(register_name, (BaseRegister,), {})
    instance = cls()
    descriptors = list(parse_bits.aggregate(l))
    code_ix = _descriptor_indicies[register_name]
    for descriptor in descriptors:
        setattr(instance, descriptor.field_name,
                partial(instance.get, code_ix=code_ix, descriptor=descriptor))
        setattr(instance, f"_{descriptor.field_name}",
                partial(instance.get_name, code_ix=code_ix, descriptor=descriptor))
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

def disassemble_addr_inner(register_const, address, const, rel):
    assert rel == 0
    if const == register_const.TEMPORARY:
        if address & (1 << 7):
            value = address & 0x7f
            return f"float({value})"
        else:
            return f"temp[{address}]"
    elif const == register_const.CONSTANT:
        return f"const[{address}]"
    else:
        assert False, const


alu_swizzle_strs = ['r', 'g', 'b', 'a', '0', 'h', '1', '_']
alu_sel_strs = ['0', '1', '2', 'p']

def disassemble_addr(register, code, suffix):
    addr0 = register.ADDR0(code)
    addr0_const = register.ADDR0_CONST(code)
    addr0_rel = register.ADDR0_REL(code)
    addr1 = register.ADDR1(code)
    addr1_const = register.ADDR1_CONST(code)
    addr1_rel = register.ADDR1_REL(code)
    addr2 = register.ADDR2(code)
    addr2_const = register.ADDR2_CONST(code)
    addr2_rel = register.ADDR2_REL(code)
    _, srcp_op, _ = register._SRCP_OP(code)

    s0 = disassemble_addr_inner(register.ADDR0_CONST, addr0, addr0_const, addr0_rel)
    s1 = disassemble_addr_inner(register.ADDR1_CONST, addr1, addr1_const, addr1_rel)
    s2 = disassemble_addr_inner(register.ADDR2_CONST, addr2, addr2_const, addr2_rel)
    sp = srcp_op.lower()
    return [
        f"src{alu_sel_strs[i]}.{suffix} = {s}"
        for i, s in enumerate([s0, s1, s2, sp])
    ]

def mod_str(s, mod):
    if mod == 0: # NOP
        return s
    elif mod == 1: # NEG
        return f"-{s}"
    elif mod == 2: # ABS
        return f"|{s}|"
    elif mod == 3: # NAB
        return f"-|{s}|"
    else:
        assert False, mod

def disassemble_rgb_swizzle_sel(code):
    rgb_sel_a = US_ALU_RGB_INST.RGB_SEL_A(code)
    red_swiz_a = US_ALU_RGB_INST.RED_SWIZ_A(code)
    green_swiz_a = US_ALU_RGB_INST.GREEN_SWIZ_A(code)
    blue_swiz_a = US_ALU_RGB_INST.BLUE_SWIZ_A(code)
    rgb_mod_a = US_ALU_RGB_INST.RGB_MOD_A(code)

    rgb_sel_b = US_ALU_RGB_INST.RGB_SEL_B(code)
    red_swiz_b = US_ALU_RGB_INST.RED_SWIZ_B(code)
    green_swiz_b = US_ALU_RGB_INST.GREEN_SWIZ_B(code)
    blue_swiz_b = US_ALU_RGB_INST.BLUE_SWIZ_B(code)
    rgb_mod_b = US_ALU_RGB_INST.RGB_MOD_B(code)

    rgb_sel_c = US_ALU_RGBA_INST.RGB_SEL_C(code)
    red_swiz_c = US_ALU_RGBA_INST.RED_SWIZ_C(code)
    green_swiz_c = US_ALU_RGBA_INST.GREEN_SWIZ_C(code)
    blue_swiz_c = US_ALU_RGBA_INST.BLUE_SWIZ_C(code)
    rgb_mod_c = US_ALU_RGBA_INST.RGB_MOD_C(code)

    rgb_swiz_a = ''.join(alu_swizzle_strs[n] for n in [red_swiz_a, green_swiz_a, blue_swiz_a])
    rgb_swiz_b = ''.join(alu_swizzle_strs[n] for n in [red_swiz_b, green_swiz_b, blue_swiz_b])
    rgb_swiz_c = ''.join(alu_swizzle_strs[n] for n in [red_swiz_c, green_swiz_c, blue_swiz_c])

    rgb_swiz = [rgb_swiz_a, rgb_swiz_b, rgb_swiz_c]
    rgb_sels = [rgb_sel_a, rgb_sel_b, rgb_sel_c]
    rgb_mods = [rgb_mod_a, rgb_mod_b, rgb_mod_c]

    return [mod_str(f"src{alu_sel_strs[sel]}.{swiz}", mod)
            for swiz, sel, mod in zip(rgb_swiz, rgb_sels, rgb_mods)], rgb_sels

def disassemble_a_swizzle_sel(code):
    alpha_sel_a = US_ALU_ALPHA_INST.ALPHA_SEL_A(code)
    alpha_swiz_a = US_ALU_ALPHA_INST.ALPHA_SWIZ_A(code)
    alpha_mod_a = US_ALU_ALPHA_INST.ALPHA_MOD_A(code)

    alpha_sel_b = US_ALU_ALPHA_INST.ALPHA_SEL_B(code)
    alpha_swiz_b = US_ALU_ALPHA_INST.ALPHA_SWIZ_B(code)
    alpha_mod_b = US_ALU_ALPHA_INST.ALPHA_MOD_B(code)

    alpha_sel_c = US_ALU_RGBA_INST.ALPHA_SEL_C(code)
    alpha_swiz_c = US_ALU_RGBA_INST.ALPHA_SWIZ_C(code)
    alpha_mod_c = US_ALU_RGBA_INST.ALPHA_MOD_C(code)

    a_swiz = [alu_swizzle_strs[n] for n in [alpha_swiz_a, alpha_swiz_b, alpha_swiz_c]]
    a_sels = [alpha_sel_a, alpha_sel_b, alpha_sel_c]
    a_mods = [alpha_mod_a, alpha_mod_b, alpha_mod_c]

    return [mod_str(f"src{alu_sel_strs[sel]}.{swiz}", mod)
            for swiz, sel, mod in zip(a_swiz, a_sels, a_mods)], a_sels

def omod_str(mod):
    if mod == 0: # * 1
        #return f"1.0 * "
        return ""
    elif mod == 1: # * 2
        return f"2.0 * "
    elif mod == 2: # * 4
        return f"4.0 * "
    elif mod == 3: # * 8
        return f"8.0 * "
    elif mod == 4: # / 2
        return f"0.5 * "
    elif mod == 5: # / 4
        return f"0.25 * "
    elif mod == 6:
        return f"0.125 * "
    elif mod == 7: # DISABLE OMOD
        return "(DISABLE OMOD) "

def disassemble_alu_dest(code):
    a_addrd = US_ALU_ALPHA_INST.ALPHA_ADDRD(code)
    a_addrd_rel = US_ALU_ALPHA_INST.ALPHA_ADDRD_REL(code)
    assert a_addrd_rel == 0

    rgb_addrd = US_ALU_RGBA_INST.RGB_ADDRD(code)
    rgb_addrd_rel = US_ALU_RGBA_INST.RGB_ADDRD_REL(code)
    assert rgb_addrd_rel == 0

    rgb_wmask, rgb_wmask_str, _ = US_CMN_INST._RGB_WMASK(code)
    a_wmask, a_wmask_str, _ = US_CMN_INST._ALPHA_WMASK(code)

    rgb_omask, rgb_omask_str, _ = US_CMN_INST._RGB_OMASK(code)
    a_omask, a_omask_str, _ = US_CMN_INST._ALPHA_OMASK(code)

    rgb_target = US_ALU_RGB_INST.TARGET(code)
    a_target = US_ALU_ALPHA_INST.TARGET(code)

    if a_omask == 0:
        assert a_target == 0
    if rgb_omask == 0:
        assert rgb_target == 0
    if a_wmask == 0:
        assert a_addrd == 0
    if rgb_wmask == 0:
        assert rgb_addrd == 0

    a_out_str = f"out[{a_target}].{a_omask_str.lower().ljust(4)} = " if a_omask != 0 else ""
    a_temp_str = f"temp[{a_addrd}].{a_wmask_str.lower().ljust(4)} = " if a_wmask != 0 else ""

    rgb_out_str = f"out[{rgb_target}].{rgb_omask_str.lower().ljust(4)} = " if rgb_omask != 0 else ""
    rgb_temp_str = f"temp[{rgb_addrd}].{rgb_wmask_str.lower().ljust(4)} = " if rgb_wmask != 0 else ""

    return (a_out_str, a_temp_str), (rgb_out_str, rgb_temp_str)

def assert_zeros_common(code):
    rgb_pred_sel = US_CMN_INST.RGB_PRED_SEL(code)
    assert rgb_pred_sel == 0
    rgb_pred_inv = US_CMN_INST.RGB_PRED_INV(code)
    assert rgb_pred_inv == 0
    write_inactive = US_CMN_INST.WRITE_INACTIVE(code)
    assert write_inactive == 0
    last = US_CMN_INST.LAST(code)
    assert last == 0
    alu_wait = US_CMN_INST.ALU_WAIT(code)
    assert alu_wait == 0
    alu_result_sel = US_CMN_INST.ALU_RESULT_SEL(code)
    assert alu_result_sel == 0
    alpha_pred_inv = US_CMN_INST.ALPHA_PRED_INV(code)
    assert alpha_pred_inv == 0
    alu_result_op = US_CMN_INST.ALU_RESULT_OP(code)
    assert alu_result_op == 0
    alpha_pred_sel = US_CMN_INST.ALPHA_PRED_SEL(code)
    assert alpha_pred_sel == 0
    stat_we = US_CMN_INST.STAT_WE(code)
    assert stat_we == 0

def assert_zeros_alu(code):
    alu_wmask = US_ALU_RGB_INST.ALU_WMASK(code)
    assert alu_wmask == 0
    w_omask = US_ALU_ALPHA_INST.W_OMASK(code)
    assert w_omask == 0

def assert_zeros_tex(code):
    dx_addr = US_TEX_ADDR_DXDY.DX_ADDR(code)
    dx_addr_rel = US_TEX_ADDR_DXDY.DX_ADDR_REL(code)
    dx_s_swiz = US_TEX_ADDR_DXDY.DX_S_SWIZ(code)
    dx_t_swiz = US_TEX_ADDR_DXDY.DX_T_SWIZ(code)
    dx_r_swiz = US_TEX_ADDR_DXDY.DX_R_SWIZ(code)
    dx_q_swiz = US_TEX_ADDR_DXDY.DX_Q_SWIZ(code)

    assert dx_addr == 0, dx_addr
    assert dx_addr_rel == 0, dx_addr_rel
    assert dx_s_swiz == 0, dx_s_swiz
    assert dx_t_swiz == 0, dx_t_swiz
    assert dx_r_swiz == 0, dx_r_swiz
    assert dx_q_swiz == 0, dx_q_swiz

    dy_addr = US_TEX_ADDR_DXDY.DY_ADDR(code)
    dy_addr_rel = US_TEX_ADDR_DXDY.DY_ADDR_REL(code)
    dy_s_swiz = US_TEX_ADDR_DXDY.DY_S_SWIZ(code)
    dy_t_swiz = US_TEX_ADDR_DXDY.DY_T_SWIZ(code)
    dy_r_swiz = US_TEX_ADDR_DXDY.DY_R_SWIZ(code)
    dy_q_swiz = US_TEX_ADDR_DXDY.DY_Q_SWIZ(code)

    assert dy_addr == 0, dy_addr
    assert dy_addr_rel == 0, dy_addr_rel
    assert dy_s_swiz == 0, dy_s_swiz
    assert dy_t_swiz == 0, dy_t_swiz
    assert dy_r_swiz == 0, dy_r_swiz
    assert dy_q_swiz == 0, dy_q_swiz

    src_addr_rel = US_TEX_ADDR.SRC_ADDR_REL(code)
    assert src_addr_rel == 0, src_addr_rel

    dst_addr_rel = US_TEX_ADDR.DST_ADDR_REL(code)
    assert dst_addr_rel == 0, dst_addr_rel

    ignore_uncovered = US_TEX_INST.IGNORE_UNCOVERED(code)
    assert ignore_uncovered == 0, ignore_uncovered

    unscaled = US_TEX_INST.UNSCALED(code)
    assert unscaled == 0, unscaled

_rgb_op_operands = {
    "OP_MAD": 3,
    "OP_DP3": 2,
    "OP_DP4": 2,
    "OP_D2A": 3,
    "OP_MIN": 2,
    "OP_MAX": 2,
    "OP_CND": 3,
    "OP_CMP": 3,
    "OP_FRC": 1,
    "OP_SOP": 0,
    "OP_MDH": 3,
    "OP_MDV": 3,
}

_a_op_operands = {
    "OP_MAD": 3,
    "OP_DP": 0,
    "OP_MIN": 2,
    "OP_MAX": 2,
    "OP_CND": 3,
    "OP_CMP": 3,
    "OP_FRC": 1,
    "OP_EX2": 1,
    "OP_LN2": 1,
    "OP_RCP": 1,
    "OP_RSQ": 1,
    "OP_SIN": 1,
    "OP_COS": 1,
    "OP_MDH": 3,
    "OP_MDV": 3
}

def disassemble_alu(code, is_output):
    assert_zeros_common(code)
    assert_zeros_alu(code)

    a_addr_strs = disassemble_addr(US_ALU_ALPHA_ADDR, code, "a")
    rgb_addr_strs = disassemble_addr(US_ALU_RGB_ADDR, code, "rgb")

    a_swizzle_sel, a_sels = disassemble_a_swizzle_sel(code)
    rgb_swizzle_sel, rgb_sels = disassemble_rgb_swizzle_sel(code)
    #print(", ".join([*rgb_swizzle_sel, *a_swizzle_sel]))

    type = US_CMN_INST.TYPE(code)
    tex_sem_wait = US_CMN_INST.TEX_SEM_WAIT(code)
    nop = US_CMN_INST.NOP(code)

    _, a_op, _ = US_ALU_ALPHA_INST._ALPHA_OP(code)
    _, rgb_op, _ = US_ALU_RGBA_INST._RGB_OP(code)

    a_op_operands = _a_op_operands[a_op]
    rgb_op_operands = _rgb_op_operands[rgb_op]

    (a_out_str, a_temp_str), (rgb_out_str, rgb_temp_str) = disassemble_alu_dest(code)

    tags = []
    assert type in {0, 1}, type
    if type == 1:
        tags.append("OUT")
    if tex_sem_wait:
        tags.append("TEX_SEM_WAIT")
    if nop:
        tags.append("NOP")
    if tags:
        print(" ".join(tags))

    if not VERBOSE:
        a_swizzle_sel = a_swizzle_sel[:a_op_operands]
        rgb_swizzle_sel = rgb_swizzle_sel[:rgb_op_operands]

        a_sources = set(a_sels)
        if 3 in a_sources:
            a_sources.add(0)
            a_sources.add(1)
        rgb_sources = set(rgb_sels)
        if 3 in rgb_sources:
            rgb_sources.add(0)
            rgb_sources.add(1)
        a_addr_strs = [s for i, s in enumerate(a_addr_strs) if i in a_sources]
        rgb_addr_strs = [s for i, s in enumerate(rgb_addr_strs) if i in rgb_sources]

    rgb_clamp = US_CMN_INST.RGB_CLAMP(code)
    alpha_clamp = US_CMN_INST.ALPHA_CLAMP(code)
    rgb_clamp_str = ".SAT" if rgb_clamp != 0 else ""
    a_clamp_str = ".SAT" if alpha_clamp != 0 else ""

    rgb_omod = US_ALU_RGB_INST.OMOD(code)
    a_omod = US_ALU_ALPHA_INST.OMOD(code)
    rgb_omod_str = omod_str(rgb_omod)
    a_omod_str = omod_str(a_omod)

    print(", ".join([*a_addr_strs, *rgb_addr_strs]), ":")
    #print(", ".join(a_addr_strs), ":")
    print(f"  {a_out_str}{a_temp_str}{a_omod_str}{a_op.removeprefix('OP_').ljust(3)}{a_clamp_str} {' '.join(a_swizzle_sel)}", ",")

    #print(", ".join(rgb_addr_strs), ":")
    print(f"  {rgb_out_str}{rgb_temp_str}{rgb_omod_str}{rgb_op.removeprefix('OP_').ljust(3)}{rgb_clamp_str} {' '.join(rgb_swizzle_sel)}", ";")

def disassemble_tex_swizzle_str(code):
    tex_swiz_strs = ["r", "g", "b", "a"]

    src_s_swiz = US_TEX_ADDR.SRC_S_SWIZ(code)
    src_t_swiz = US_TEX_ADDR.SRC_T_SWIZ(code)
    src_r_swiz = US_TEX_ADDR.SRC_R_SWIZ(code)
    src_q_swiz = US_TEX_ADDR.SRC_Q_SWIZ(code)

    src_swiz = ''.join(tex_swiz_strs[n] for n in [src_s_swiz, src_t_swiz, src_r_swiz, src_q_swiz])

    dst_r_swiz = US_TEX_ADDR.DST_R_SWIZ(code)
    dst_g_swiz = US_TEX_ADDR.DST_G_SWIZ(code)
    dst_b_swiz = US_TEX_ADDR.DST_B_SWIZ(code)
    dst_a_swiz = US_TEX_ADDR.DST_A_SWIZ(code)

    dst_swiz = ''.join(tex_swiz_strs[n] for n in [dst_r_swiz, dst_g_swiz, dst_b_swiz, dst_a_swiz])

    return src_swiz, dst_swiz

def disassemble_tex_dest(code):
    dst_addr = US_TEX_ADDR.DST_ADDR(code)

    rgb_wmask, rgb_wmask_str, _ = US_CMN_INST._RGB_WMASK(code)
    a_wmask, a_wmask_str, _ = US_CMN_INST._ALPHA_WMASK(code)
    wmask_bool = rgb_wmask != 0 or a_wmask != 0

    rgba_wmask = (rgb_wmask_str if rgb_wmask else "") + (a_wmask_str if a_wmask else "")

    temp_str = f"temp[{dst_addr}].{rgba_wmask.lower().ljust(4)} = " if wmask_bool else ""

    rgb_omask, rgb_omask_str, _ = US_CMN_INST._RGB_OMASK(code)
    a_omask, a_omask_str, _ = US_CMN_INST._ALPHA_OMASK(code)

    assert rgb_omask == 0
    assert a_omask == 0
    #omask_bool = rgb_omask != 0 or a_omask != 0
    #rgba_omask = (a_omask_str if a_omask else "") + (rgb_omask_str if rgb_omask else "")
    #out_str = f"out[{dst_addr}].{rgba_omask.lower().ljust(4)} = " if omask_bool else ""

    return temp_str

def disassemble_tex(code):
    assert_zeros_common(code)
    assert_zeros_tex(code)

    _, inst, _ = US_TEX_INST._INST(code)
    tex_id = US_TEX_INST.TEX_ID(code)

    src_addr = US_TEX_ADDR.SRC_ADDR(code)

    src_swiz, dst_swiz = disassemble_tex_swizzle_str(code)
    temp_str = disassemble_tex_dest(code)

    tags = ["TEX"]
    if US_CMN_INST.TEX_SEM_WAIT(code):
        tags.append("TEX_SEM_WAIT")
    if US_TEX_INST.TEX_SEM_ACQUIRE(code):
        tags.append("TEX_SEM_ACQUIRE")
    if US_CMN_INST.ALU_WAIT(code):
        tags.append("ALU_WAIT")

    print(" ".join(tags))
    print(f"  {temp_str}{inst} tex[{tex_id}].{dst_swiz} temp[{src_addr}].{src_swiz} ;")

def disassemble(code):
    assert len(code) == 6, len(code)
    type = US_CMN_INST.TYPE(code)
    if type == US_CMN_INST.TYPE.US_INST_TYPE_OUT:
        disassemble_alu(code, is_output=True)
    elif type == US_CMN_INST.TYPE.US_INST_TYPE_ALU:
        disassemble_alu(code, is_output=False)
    elif type == US_CMN_INST.TYPE.US_INST_TYPE_TEX:
        disassemble_tex(code)
    else:
        print("[TYPE]", type)
        #assert False, US_CMN_INST._TYPE(code)

def parse_hex(s):
    assert s.startswith('0x')
    return int(s.removeprefix('0x'), 16)

if __name__ == "__main__":
    filename = sys.argv[1]
    if filename.endswith(".bin"):
        with open(filename, 'rb') as f:
            buf = f.read()
        code = [struct.unpack("<I", buf[i*4:i*4+4])[0] for i in range(len(buf) // 4)]
    else:
        with open(filename) as f:
            buf = f.read()
        code = [parse_hex(c.strip()) for c in buf.split(',') if c.strip()]

    for i in range(len(code) // 6):
        start = (i + 0) * 6
        end   = (i + 1) * 6
        disassemble(code[start:end])
        print()
