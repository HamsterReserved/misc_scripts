#!/usr/bin/python3

# Usage: yaml_merger.py <output file> <source file> [[file 1] [file 2] ... ]
# Squash all file X on the cmdline into the source file and write to output.

# Keep keys in output file in the same order as input files.
import oyaml as yaml
import sys

if len(sys.argv) < 3:
    print("At least 2 arguments required.")
    print("""
Usage: yaml_merger.py <output file> <source file> [[file 1] [file 2] ... ]
Squash all file X into the source file and write to output file.
""")
    exit(-1)

FINAL_DICT = {}

for path in sys.argv[2:]:
    with open(path, "r") as f:
        new_dict = yaml.safe_load(f)
        FINAL_DICT.update(new_dict)

with open(sys.argv[1], "w") as f:
    yaml.safe_dump(FINAL_DICT, f, allow_unicode=True)
