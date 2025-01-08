#!/usr/bin/env python3

import sys

def bin2cpp(input_file, output_file):
    with open(input_file, 'rb') as f:
        data = f.read()
    
    with open(output_file, 'w') as f:
        f.write("// Auto-generated file - do not edit\n")
        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("const uint8_t pd_block_firmware[256] = {\n    ")
        for i, byte in enumerate(data):
            if i > 0 and i % 16 == 0:
                f.write("\n    ")
            f.write("0x{:02X}, ".format(byte))
        f.write("\n};\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: {} <input_binary> <output_cpp>".format(sys.argv[0]))
        sys.exit(1)
    bin2cpp(sys.argv[1], sys.argv[2]) 