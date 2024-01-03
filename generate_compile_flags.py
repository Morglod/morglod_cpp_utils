import os

includes = []

ROOT_SRC_DIR = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "./src")).replace("\\", "/")

with open("./build_tmp/conanbuildinfo.txt", 'rt') as buildinfo:
    lines = buildinfo.readlines()
    reading_includes = False

    for i in range(0, len(lines)):
        if lines[i] == "[includedirs]\n":
            reading_includes = True
            continue
        if reading_includes and (lines[i].startswith("[") or lines[i] == "\n"):
            break
        if reading_includes:
            includes.append(lines[i])

includes.append(ROOT_SRC_DIR)

with open("./compile_flags.txt", "wt") as compile_flags:
    compile_flags.write("-std=c++20\n")
    for x in includes:
        compile_flags.write("-I" + x)
