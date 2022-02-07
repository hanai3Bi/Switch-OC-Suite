#!python

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
#   Values should be > 1600000, and divided evenly by 9600 or 12800.
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

cust_range = {"mtcConf":           (0, 3),
              "marikoCpuMaxClock": (1785000, 3000000),
              "marikoCpuMaxVolt":  (1100, 1300),
              "marikoGpuMaxClock": (768000, 1536000),
              "marikoEmcMaxClock": (1612800, 2400000),
              "eristaCpuMaxVolt":  (1100, 1400),
              "eristaEmcMaxClock": (1600000, 2400000),
              "eristaEmcVolt":     (1100000, 1250000)}

import struct
import csv
from pprint import pprint
import argparse

cust_rev   = 1
cust_head  = ["cust", "custRev"]
cust_body  = ["mtcConf",
              "marikoCpuMaxClock", "marikoCpuMaxVolt", "marikoGpuMaxClock", "marikoEmcMaxClock",
              "eristaCpuOCEnable", "eristaCpuMaxVolt", "eristaEmcMaxClock", "eristaEmcVolt"]
cust_key   = [*cust_head, *cust_body]

parser = argparse.ArgumentParser(description='Loader Configurator v'+str(cust_rev))
parser.add_argument("file", help="Path of loader.kip")
parser.add_argument("--save", "-s", action="store_true", help="Save configuration to loader.kip")
parser.add_argument("--ignore", action="store_true", help="Ignore range safety check")
args = parser.parse_args()

def CSVRead(file_loc):
  with open(file_loc, 'r') as file:
    rd = csv.reader(file)
    dct = {rows[0]:rows[1] for rows in rd}
    file.close()
    return dct

def CSVWrite(file_loc, dct):
  with open(file_loc, 'w') as file:
    for key, value in dct.items():
      file.write('{0},{1}\n'.format(key, value))
    file.close()

def CustSafetyCheck(cust):
  warn_cnt = 0
  for i in cust_body:
    val = int(cust[i])
    if val and val not in range(*cust_range[i]):
      warn_cnt += 1
      print("[!] %s = %u (Expected range: %u ≤ value ≤ %u)" % (i, val, *cust_range[i]))
  return warn_cnt

def KIPCustHandler(file_loc, cust={}):
  if len(cust) != 0:
    missing = set(cust_body) - set(cust.keys())
    if missing:
      print("Invalid cust dict! Missing: %s" % missing)
      return
    if CustSafetyCheck(cust) and args.ignore == False:
      return

  with open(file_loc, "rb") as file:
    header_str = b'KIP1Loader'
    header     = file.read(len(header_str))
    file.seek(0)
    cust_magic = b'CUST'
    cust_pos   = file.read().find(cust_magic)

    if header != header_str or cust_pos == -1:
      print("Invalid kip file!")
      return False

    file.seek(cust_pos)
    cust_fmt  = '<4s2H8I'
    cust_size = struct.calcsize(cust_fmt)
    cust_buf  = file.read(cust_size)
    cust_val  = struct.unpack(cust_fmt, cust_buf)
    cust_dict = dict(zip(cust_key, cust_val))
    file.close()

    if cust_dict['custRev'] != cust_rev:
      print("custRev does NOT match, expected: %u, got: %u!" % (cust_rev, cust_dict['custRev']))
      return False

    if cust == {}:
      [cust_dict.pop(key) for key in cust_head]
      return cust_dict

  with open(file_loc, "rb+") as file:
    cust_head_fmt = '<4s1H'
    cust_body_fmt = '<1H8I'
    cust_bin = struct.pack(cust_body_fmt, *[cust[i] for i in cust_body])
    file.seek(cust_pos + struct.calcsize(cust_head_fmt))
    file.write(cust_bin)
    file.close()
    print("Done!")

def main(file_loc, save=False):
  cust = KIPCustHandler(file_loc)
  if not cust:
    return
  print("Configuration from file:")
  pprint(cust, sort_dicts=False)

  if save:
    print("Saving new configuration...:")
    pprint(cust_conf, sort_dicts=False)
    KIPCustHandler(file_loc, cust_conf)

if __name__ == "__main__":
  main(args.file, args.save)
