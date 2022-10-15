#!python3

import os
import struct
import sys

try:
    fpath = sys.argv[1]
except IndexError:
    raise SystemExit(f"Usage: {sys.argv[0]} <path>")

file = open(fpath, "rb")
file.seek(0)
file_size = int.from_bytes(file.read(4), byteorder="little")

domain_key_value = {}

while file.tell() != file_size:
    domain_key_len = int.from_bytes(file.read(4), byteorder="little")
    domain_key = file.read(domain_key_len).decode().rstrip('\x00')
    domain, key = domain_key.split("!")

    if not domain_key_value.get(domain):
        domain_key_value[domain] = {}

    val_types = {1: "str", 2: "u8", 3: "u32"}
    val_type = val_types.get(int.from_bytes(file.read(1), byteorder="little"), "unknown")
    val_len = int.from_bytes(file.read(4), byteorder="little")
    val = file.read(val_len)

    if val_type == "str":
        value = '"' + val.decode().rstrip('\x00') + '"'
    else:
        val_dec = int.from_bytes(val, byteorder="little")
        value = "0x" + format(val_dec, 'X')
        if val_type == "u32" and val_dec >= 10:
            val_dec = int.from_bytes(val, byteorder="little", signed=True)
            value += f" ; {val_dec}"

    domain_key_value[domain][key] = f"{val_type}!{value}"

for domain in sorted(domain_key_value.keys()):
    print(f"[{domain}]")
    for key in sorted(domain_key_value[domain].keys()):
        print(f"{key} = {domain_key_value[domain][key]}")
    print()
