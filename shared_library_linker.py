#!/usr/bin/python3

import sys
import os

# pip3 install pyelftools, https://github.com/eliben/pyelftools
from elftools.elf.elffile import ELFFile
from elftools.elf.dynamic import DynamicSection

LIBRARY_PATHS = []
ALL_LIBRARIES = {}


def get_needed_libraries(path):
    with open(path, "rb") as f:
        elf = None
        try:
            elf = ELFFile(f)
        except:
            print("ERROR: %s is not a valid ELF file" % (path,))
            return []

        dyna_sect = elf.get_section_by_name(".dynamic")

        if not isinstance(dyna_sect, DynamicSection):
            print("ERROR: %s does not have a valid dynamic section." % (path,))
            return []

        shared_libs = []
        for shlib in dyna_sect.iter_tags("DT_NEEDED"):
            shared_libs.append(shlib.needed)
        return shared_libs


def find_library(libname):
    if libname in ALL_LIBRARIES.keys():
        return ALL_LIBRARIES[libname]
    else:
        return None


if __name__ == "__main__":
    if len(sys.argv) < 2 or (sys.argv[2] == "-a" and len(sys.argv) < 5):
        print("Usage: %s <target library> [[-a32 <Android system path> <Android vendor path>] | [library path] [library path]...]")
        print("Find all direct and intermediate dependencies for target library in library paths.")
        print("If not library path defined, current working directory will be searched.")
        print("If -a is given, library paths will be generated automatically.")
        exit(1)

    if sys.argv[2] == "-a":
        bits_suffix = ""
        with open(sys.argv[1], "rb") as f:
            elf = ELFFile(f)
            if elf.elfclass == 64:
                bits_suffix = "64"
        LIBRARY_PATHS.append(sys.argv[4] + "/lib" + bits_suffix)
        LIBRARY_PATHS.append(sys.argv[4] + "/lib" + bits_suffix + "/hw")
        LIBRARY_PATHS.append(sys.argv[3] + "/lib" + bits_suffix)
        LIBRARY_PATHS.append(sys.argv[3] + "/lib" + bits_suffix + "/hw")
    else:
        LIBRARY_PATHS += sys.argv[2:]
        if len(LIBRARY_PATHS) == 0:
            LIBRARY_PATHS.append(".")

    for path in LIBRARY_PATHS:
        for file in os.listdir(path):
            fullpath = path + "/" + file
            if os.path.isfile(fullpath):
                ALL_LIBRARIES[file] = fullpath

    valid_dependencies = {}
    unresolved_dependencies = set()
    pending_dependencies = {}

    pending_dependencies[sys.argv[1]] = sys.argv[1]
    while len(pending_dependencies.keys()) > 0:
        new_pending = {}
        for lib in pending_dependencies.keys():
            curr_dependencies = get_needed_libraries(pending_dependencies[lib])
            for d in curr_dependencies:
                if d not in valid_dependencies.keys() and d not in unresolved_dependencies:
                    # Not known present, neither missing, needs analyzing
                    path = find_library(d)
                    if path is not None:
                        new_pending[d] = path
                        valid_dependencies[d] = path
                    else:
                        unresolved_dependencies.add(d)
        pending_dependencies = new_pending

    print("Valid dependencies:")
    for k in valid_dependencies:
        print("%s: %s" % (k, valid_dependencies[k]))

    print("")
    print("Unresolved dependcies:")
    for k in unresolved_dependencies:
        print(k)
