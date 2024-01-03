import glob
import os

ROOT_DIR = os.path.dirname(os.path.realpath(__file__))

SRC_FILES = os.path.join(ROOT_DIR, "./src/**/*.cpp")
FOUND_FILES = glob.glob(SRC_FILES, recursive=True)

with open(os.path.join(ROOT_DIR, "./src/CMakeLists.txt"), 'w') as out:
    out.write("add_executable(main ")
    for file in FOUND_FILES:
        out.write("\n./" + os.path.relpath(os.path.realpath(file), ROOT_DIR).replace("\\", "/"))
    out.write("\n)")
