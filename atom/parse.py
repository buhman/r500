import sys
import struct
from opcodes import opcodes, i_to_str
import types

with open(sys.argv[1], 'rb') as f:
    buf = f.read()
    rom = memoryview(buf)

def u8(offset):
    global rom
    value, = struct.unpack("<B", rom[offset:offset+1])
    return value

def u16(offset):
    global rom
    value, = struct.unpack("<H", rom[offset:offset+2])
    return value

def u32(offset):
    global rom
    value, = struct.unpack("<I", rom[offset:offset+4])
    return value

def rstr(offset, length):
    global rom
    return bytes(rom[offset:offset + length])

def uN(offset, N):
    if N == 1:
        return u8(offset)
    if N == 2:
        return u16(offset)
    if N == 4:
        return u32(offset)
    assert False, N

ATOM_BIOS_MAGIC = 0xaa55
ATOM_ATI_MAGIC_OFFSET = 0x30
ATOM_ATI_MAGIC = b" 761295520"
ATOM_ROM_TABLE_OFFSET = 0x48
ATOM_ROM_MAGIC = b"ATOM"

ATOM_ROM_MAGIC_OFFSET = 0x04
ATOM_ROM_MSG_OFFSET   = 0x10
ATOM_ROM_CMD_OFFSET   = 0x1e
ATOM_ROM_DATA_OFFSET  = 0x20

_command_table_names = [
    "ASIC_Init",
    "GetDisplaySurfaceSize",
    "ASIC_RegistersInit",
    "VRAM_BlockVenderDetection",
    "DIGxEncoderControl",
    "MemoryControllerInit",
    "EnableCRTCMemReq",
    "MemoryParamAdjust",
    "DVOEncoderControl",
    "GPIOPinControl",
    "SetEngineClock",
    "SetMemoryClock",
    "SetPixelClock",
    "DynamicClockGating",
    "ResetMemoryDLL",
    "ResetMemoryDevice",
    "MemoryPLLInit",
    "AdjustDisplayPll",
    "AdjustMemoryController",
    "EnableASIC_StaticPwrMgt",
    "ASIC_StaticPwrMgtStatusChange",
    "DAC_LoadDetection",
    "LVTMAEncoderControl",
    "LCD1OutputControl",
    "DAC1EncoderControl",
    "DAC2EncoderControl",
    "DVOOutputControl",
    "CV1OutputControl",
    "GetConditionalGoldenSetting",
    "TVEncoderControl",
    "TMDSAEncoderControl",
    "LVDSEncoderControl",
    "TV1OutputControl",
    "EnableScaler",
    "BlankCRTC",
    "EnableCRTC",
    "GetPixelClock",
    "EnableVGA_Render",
    "GetSCLKOverMCLKRatio",
    "SetCRTC_Timing",
    "SetCRTC_OverScan",
    "SetCRTC_Replication",
    "SelectCRTC_Source",
    "EnableGraphSurfaces",
    "UpdateCRTC_DoubleBufferRegisters",
    "LUT_AutoFill",
    "EnableHW_IconCursor",
    "GetMemoryClock",
    "GetEngineClock",
    "SetCRTC_UsingDTDTiming",
    "ExternalEncoderControl",
    "LVTMAOutputControl",
    "VRAM_BlockDetectionByStrap",
    "MemoryCleanUp",
    "ProcessI2cChannelTransaction",
    "WriteOneByteToHWAssistedI2C",
    "ReadHWAssistedI2CStatus",
    "SpeedFanControl",
    "PowerConnectorDetection",
    "MC_Synchronization",
    "ComputeMemoryEnginePLL",
    "MemoryRefreshConversion",
    "VRAM_GetCurrentInfoBlock",
    "DynamicMemorySettings",
    "MemoryTraining",
    "EnableSpreadSpectrumOnPPLL",
    "TMDSAOutputControl",
    "SetVoltage",
    "DAC1OutputControl",
    "DAC2OutputControl",
    "SetupHWAssistedI2CStatus",
    "ClockSource",
    "MemoryDeviceInit",
    "EnableYUV",
    "DIG1EncoderControl",
    "DIG2EncoderControl",
    "DIG1TransmitterControl",
    "DIG2TransmitterControl",
    "ProcessAuxChannelTransaction",
    "DPEncoderService",
]

# attr

# 2:0 - ATOM_ARG_
# 5:3 - ATOM_SRC_
# 7:6 - dest align
#     long: 0 [31-0]
#     word: 0 [15-0]
#           1 [23-8]
#           2 [31-16]
#     byte  0 [7-0]
#           1 [15-8]
#           2 [23-16]
#           3 [31-24]

def opcode_type_setport(offset, dest_type):
    if dest_type == "ati":
        return 2
    elif dest_type == "pci":
        return 1
    elif dest_type == "sysio":
        return 1
    else:
        assert False, dest_type

def src_size(src):
    return [
        4, # DWORD
        2, # WORD0
        2, # WORD8
        2, # WORD16
        1, # BYTE0
        1, # BYTE8
        1, # BYTE16
        1, # BYTE24
    ][src]

SRC = types.SimpleNamespace()
SRC.REG = 0
SRC.PS = 1
SRC.WS = 2
SRC.FB = 3
SRC.ID = 4
SRC.IMM = 5
SRC.PLL = 6
SRC.MC = 7

def dest_arg_size(dest_type):
    assert type(dest_type) == int
    match dest_type:
        case SRC.REG:
            return 2
        case SRC.IMM:
            assert False, dest_type
        case SRC.ID:
            assert False, dest_type
        case _:
            return 1

def dest_src_size(arg, src):
    assert type(arg) == int
    match arg:
        case SRC.IMM:
            return src_size(src)
        case SRC.REG:
            return 2
        case SRC.ID:
            return 2
        case _:
            return 1

def print_size(n, size):
    if size == 1:
        print(f"{n:02x}", end='')
    elif size == 2:
        print(f"{n:04x}", end='')
    else:
        assert False, size

def opcode_type_dest_src(offset, dest_type):
    attr = u8(offset)
    arg = (attr >> 0) & 0b111
    src = (attr >> 3) & 0b111

    arg_size = dest_arg_size(dest_type)
    src_size = dest_src_size(arg, src)
    arg_value = uN(offset + 1, arg_size)
    src_value = uN(offset + 1 + arg_size, src_size)

    print_size(arg_value, arg_size)
    print(" <- ", end='')
    print_size(src_value, src_size)

    return 1 + arg_size + src_size

def opcode_type_1x16(offset, dest_type):
    arg = u16(offset)
    print_size(arg, 2)
    return 2

def opcode_type_setregblock(offset, dest_type):
    arg = u16(offset)
    print_size(arg, 2)
    return 2

def opcode_type_dest(offset, dest_type):
    attr = u8(offset)
    src = (attr >> 3) & 0b111
    src_size = dest_src_size(dest_type, src)
    src_arg = uN(offset + 1, src_size)

    print_size(src_arg, src_size)

    return 1 + src_size

def opcode_type_shift(offset, dest_type):
    attr = u8(offset)
    src = (attr >> 3) & 0b111
    src_size = dest_src_size(dest_type, src)
    src_arg = uN(offset + 1, src_size)

    print_size(src_arg, src_size)

    shift_arg = u8(offset + 1 + src_size)

    print(" by ", end='')
    print_size(shift_arg, 1)

    return 1 + src_size + 1

def opcode_0(offset, dest_type):
    return 0

argument_handlers = {
    "setport": opcode_type_setport,
    "dest_src": opcode_type_dest_src,
    "1x16": opcode_type_1x16,
    "setregblock": opcode_type_1x16,
    "shift": opcode_type_shift,
    "dest": opcode_type_dest,
    "0": opcode_0,
}

def disassemble(start, length):
    offset = start
    offset_end = start + length - 6

    while offset < offset_end:
        opcode = u8(offset)
        arg_type, name, dest_type = opcodes[opcode]
        pc = (offset - start) + 6
        offset += 1
        handler = argument_handlers[arg_type]
        print(f"{pc:04x} opcode {opcode:02x} {name.rjust(12)}  {i_to_str(dest_type).ljust(8)} ", end='')
        length = handler(offset, dest_type)
        offset += length
        print()

def parse_table(names, table):
    structure_size = u16(table + 0)
    format_revision = u8(table + 2)
    content_revision = u8(table + 3)
    print("structure size", structure_size)
    print("format revision", format_revision)
    print("content revision", content_revision)
    table_entries = (structure_size - 4) // 2
    for i in range(table_entries):
        offset = u16(table + 4 + i * 2)
        if offset == 0:
            print(f"{i:02x} offset {offset:04x}                                  : ({names[i]})")
            continue
        length = u16(offset + 0)
        format_rev = u8(offset + 2)
        content_rev = u8(offset + 3)
        print(f"{i:02x} offset {offset:04x} length {length:04x} format {format_rev:02x} content {content_rev:02x} : ({names[i]})")
        if i == 0x22:
            disassemble(offset + 6, length)
            break

def parse_header():
    bios_magic = u16(0)
    assert bios_magic == ATOM_BIOS_MAGIC, bios_magic
    ati_magic = rstr(ATOM_ATI_MAGIC_OFFSET, len(ATOM_ATI_MAGIC))
    assert ati_magic == ATOM_ATI_MAGIC, ati_magic

    base = u16(ATOM_ROM_TABLE_OFFSET)
    rom_magic = rstr(base + ATOM_ROM_MAGIC_OFFSET, len(ATOM_ROM_MAGIC))
    assert rom_magic == ATOM_ROM_MAGIC, rom_magic

    print("structure size", u16(0))

    cmd_table = u16(base + ATOM_ROM_CMD_OFFSET)
    data_table = u16(base + ATOM_ROM_DATA_OFFSET)

    str = rom[u16(base + ATOM_ROM_MSG_OFFSET):]
    while str[0] == ord(b'\n') or str[0] == ord(b'\r'):
        str = str[1:]
    message = []
    for i in range(511):
        if str[i] == 0:
            break
        message.append(str[i])
    message = bytes(message).strip()
    print(message)

    print("command table:")
    parse_table(_command_table_names, cmd_table)
    print("\n\n")
    #print("data table:")
    #parse_table(data_table)

parse_header()
