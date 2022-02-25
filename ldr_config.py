#!python3
cust_conf  = {
# DRAM Timing:
# 0: AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density (Default).
# 1: AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.
# 2: ENTIRE_TABLE_ERISTA:
# 3: ENTIRE_TABLE_MARIKO:  Replace the entire max mtc table with customized one (provided by user).
    "mtcConf":           0,
# Mariko CPU:
# - Max Clock in kHz:
#   Default: 1785000
#   >= 2193000 will enable overvolting (> 1120 mV)
# - Max Voltage in mV:
#   Default voltage: 1120
#   Haven't tested anything higher than 1220.
    "marikoCpuMaxClock": 2397000,
    "marikoCpuMaxVolt":  1220,
# Mariko GPU:
# - Max Clock in kHz:
#   Default: 921600
#   NVIDIA Maximum: 1267200
    "marikoGpuMaxClock": 1305600,
# Mariko EMC:
# - RAM Clock in kHz:
#   Values should be > 1600000, and divided evenly by 9600.
#   [WARNING]
#   RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM:
#   - Graphical glitches
#   - System instabilities
#   - NAND corruption
#   Timings from auto-adjustment have been tested safe for up to 1996.8 MHz for all DRAM chips.
    "marikoEmcMaxClock": 1996800,
# Erista CPU:
# Not tested and not enabled by default.
# - Enable Overclock
#   Require modificaitions towards NewCpuTables!
# - Max Voltage in mV
    "eristaCpuOCEnable": 0,
    "eristaCpuMaxVolt":  0,
# Erista EMC:
# - RAM Clock in kHz
#   [WARNING]
#   RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM:
#   - Graphical glitches
#   - System instabilities
#   - NAND corruption
# - RAM Voltage in uV
#   Range: 600'000 to 1250'000 uV
#   Value should be divided evenly by 12'500
#   Default(HOS): 1125'000
#   Not enabled by default.
    "eristaEmcMaxClock": 1862400,
    "eristaEmcVolt":     0
}

cust_range = {
    "mtcConf":           (0, 3),
    "marikoCpuMaxClock": (1785000, 3000000),
    "marikoCpuMaxVolt":  (1100, 1300),
    "marikoGpuMaxClock": (768000, 1536000),
    "marikoEmcMaxClock": (1612800, 2400000),
    "eristaCpuMaxVolt":  (1100, 1400),
    "eristaEmcMaxClock": (1600000, 2400000),
    "eristaEmcVolt":     (1100000, 1250000)
}

import struct
import argparse

cust_rev  = 1
cust_head = ["cust", "custRev"]
cust_body = ["mtcConf",
    "marikoCpuMaxClock", "marikoCpuMaxVolt", "marikoGpuMaxClock", "marikoEmcMaxClock",
    "eristaCpuOCEnable", "eristaCpuMaxVolt", "eristaEmcMaxClock", "eristaEmcVolt"]
cust_key  = [*cust_head, *cust_body]

parser = argparse.ArgumentParser(description='Loader Configurator v'+str(cust_rev))
parser.add_argument("file", help="Path of loader.kip")
parser.add_argument("--save", "-s", action="store_true", help="Save configuration to loader.kip")
parser.add_argument("--ignore", action="store_true", help="Ignore range safety check")
args = parser.parse_args()

def KIPCustParse(file_loc, conf_print=True) -> (int, dict):
    with open(file_loc, "rb") as file:
        header_str = b'KIP1Loader'
        header     = file.read(len(header_str))
        file.seek(0)
        cust_magic = b'CUST'
        cust_pos   = file.read().find(cust_magic)

        if header != header_str or cust_pos == -1:
            raise Exception("\n  Invalid kip file!")

        file.seek(cust_pos)
        cust_fmt  = '<4s2H8I'
        cust_size = struct.calcsize(cust_fmt)
        cust_buf  = file.read(cust_size)
        cust_val  = struct.unpack(cust_fmt, cust_buf)
        cust_dict = dict(zip(cust_key, cust_val))

        if cust_dict['custRev'] != cust_rev:
            raise Exception(f"\n  custRev does NOT match, expected: {cust_rev}, got: {cust_dict['custRev']}!")

        [cust_dict.pop(key) for key in cust_head]

        if conf_print:
            print("Configuration from file")
            [print(f"- {i:18s} : {cust_dict[i]:8d}") for i in cust_dict]

        return (cust_pos, cust_dict)


def CustRangeCheck(cust):
    range_error_str = ""
    for i in cust_range:
        val = int(cust[i])
        if val and (val < cust_range[i][0] or val > cust_range[i][1]) :
            range_error_str += f"\n- {i:18s} = {val:8d}, Expected range: {[*cust_range[i]]}"

    if range_error_str:
        raise ValueError(range_error_str)


def KIPCustSave(file_loc, cust_pos, cust_dict, range_check=True, cust_to_save={}):
    missing = set(cust_body) - set(cust_to_save.keys())
    if missing:
        missing_str = "\n  Invalid cust! Missing: "
        for i in missing:
            missing_str += f"\n- {i}"
        raise Exception(missing_str)

    if range_check:
        CustRangeCheck(cust_to_save)

    diff_count = 0
    for i in cust_body:
        diff_str = ""
        if cust_dict[i] != cust_conf[i]:
            diff_str = f"-> {cust_conf[i]:8d}"
            diff_count += 1
        print(f"- {i:18s} : {cust_dict[i]:8d} {diff_str}")

    if not diff_count:
        print("Cust is identical, abort saving!")
        return

    with open(file_loc, "rb+") as file:
        cust_head_fmt = '<4s1H'
        cust_body_fmt = '<1H8I'
        cust_bin = struct.pack(cust_body_fmt, *[cust_to_save[i] for i in cust_body])
        file.seek(cust_pos + struct.calcsize(cust_head_fmt))
        file.write(cust_bin)

    print("Done!")


def main(file_loc, ignore=False, save=False):
    (cust_pos, cust_dict) = KIPCustParse(file_loc, conf_print=(not save))

    if save:
        print("Saving new configuration...")
        KIPCustSave(file_loc, cust_pos, cust_dict, range_check=(not ignore), cust_to_save=cust_conf)


if __name__ == "__main__":
    main(args.file, args.ignore, args.save)
