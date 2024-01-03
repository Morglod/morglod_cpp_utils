import glob
import os
import json
import hashlib
import re
from typing import Callable

# see supported macros list in the bottom of this script

ROOT_DIR = os.path.dirname(os.path.realpath(__file__))
CACHE_FILE_PATH = os.path.join(ROOT_DIR, './process_marco_cache.json')

SRC_FILES_HPP = os.path.join(ROOT_DIR, "./src/**/*.hpp")
SRC_FILES_CPP = os.path.join(ROOT_DIR, "./src/**/*.cpp")

# rel path -> latest processed modify time
files_cache: dict = {}

def str_hash(s: str):
    return hashlib.sha1(s.encode("utf-8")).hexdigest()[:5]

class FileInfo:
    def __init__(self, file_rel_path: str, file_full_path: str, file_source: str):
        self.file_rel_path = file_rel_path
        self.file_full_path = file_full_path
        self.file_source = file_source

class MacroDef:
    def __init__(self, macro_def: str, macro_name: str, macro_cmd: str, macro_hash: str, macro_result_start: int, macro_result_end: int,  prev_result: str or None, end_ind: int):
        self.macro_def = macro_def
        self.macro_name = macro_name
        self.prev_result = prev_result
        self.macro_cmd = macro_cmd
        self.macro_hash = macro_hash
        self.macro_result_start = macro_result_start
        self.macro_result_end = macro_result_end
        self.end_ind = end_ind

        self.result_start_str = "// _generate_start_ " + macro_hash
        self.result_end_str = "// _generate_end_ " + macro_hash

    def insert_result(self, text_source: str, result: str) -> "tuple[str, int]":
        insert_str = self.result_start_str + "\n" + result + "\n" + self.result_end_str + "\n"
        at_ind = self.macro_result_start if self.macro_result_start != -1 else self.end_ind + 1
        out = replace_or_insert(text_source, insert_str, at_ind, -1 if self.macro_result_end == -1 else self.macro_result_end + len(self.result_end_str + "\n"))
        return [ out, at_ind + len(insert_str) ]

def find_macro_def(file_source: str, macro_name: str, from_index: int = 0) -> MacroDef or None:
    macro_definition = "// " + macro_name + " "
    ind = file_source.find(macro_definition, from_index)
    if (ind == -1):
        return None
    macro_line_end = file_source.find("\n", ind + 1)
    macro_cmd = file_source[ind + len(macro_definition) : macro_line_end]
    macro_hash = str_hash(macro_definition + macro_cmd)
    
    macro_result_start = file_source.find("// _generate_start_ " + macro_hash, macro_line_end + 1)
    macro_result_end = file_source.find("// _generate_end_ " + macro_hash, macro_line_end + 1)
    prev_result: str or None = None

    if (macro_result_start != -1 and macro_result_end != -1):
        prev_result = file_source[(file_source.find("\n", macro_result_start + 1) + 1) : macro_result_end]
    
    return MacroDef (
        macro_def=macro_definition,
        macro_name=macro_name,
        macro_cmd=macro_cmd,
        macro_hash=macro_hash,
        macro_result_start=macro_result_start,
        macro_result_end=macro_result_end,
        prev_result=prev_result,
        end_ind = macro_line_end if macro_result_end == -1 else file_source.find("\n", macro_result_end + 1)
    )

# substr_from_braces("(hello(123()))", "(", ")") -> "hello(123())"
# substr_from_braces("hello(123()))", "(", ")") -> "123())"
def substr_from_braces(text: str, open_brace: str, close_brace: str):
    start = text.find(open_brace)
    end = text.rfind(close_brace)
    if (start == -1 or end == -1):
        return None
    return text[start+1 : end].strip()

def extract_args_from_braces(text: str, open_brace, close_brace, delim=",", other_open_braces: "list[str]" = [ "(", "<" ], other_close_braces: "list[str]"=[ ")", ">" ]):
    text = substr_from_braces(text, open_brace, close_brace)
    if (text == None):
        return None
    depth = 0
    items: list[str] = []
    item_ind_start = 0

    for i in range(0, len(text)):
        if (text[i] == open_brace or text[i] in other_open_braces):
            depth += 1
            continue
        if (text[i] == close_brace or text[i] in other_close_braces):
            depth -= 1
            continue
        if (text[i] == delim and depth == 0):
            item = text[item_ind_start : i].strip()
            items.append(item)
            item_ind_start = i+1

    if (item_ind_start != len(text)):
        item = text[item_ind_start:].strip()
        items.append(item)
    
    return items

def extract_word(text: str, from_ind: int = 0, delim: "list[str]" = [" ", "{", "}", "(", ")", "[", "]"]) -> "tuple[ str or None, int ]":
    r = re.compile("|".join(map(lambda x: "(\\" + x + ")", delim)))
    m = r.search(text, pos=from_ind)
    if (m != None):
        return [ text[from_ind : m.start()], m.start() ]
    return [ None, from_ind ]

def replace_or_insert(text: str, insert_str: str, at_ind: int, replace_end_ind: int = -1):
    if (replace_end_ind == -1):
        replace_end_ind = at_ind
    return text[:at_ind] + insert_str + text[replace_end_ind:]

# -----------------------------------------   GENERATE UNION ENUM MACRO   --------------------------------------------

def generate_enum_code(enum_name: str, enum_items: list):
    types_type = "uint8_t" if len(enum_items) <= 255 else ("uint16_t" if len(enum_items) <= 65535 else "uint32_t")
    types_str = "\n".join(map(lambda x: "        " + x.name + ",", enum_items))
    union_str = "\n".join(map(lambda x: "        " + x.type + " " + x.name + ";", enum_items))
    switch_assign_op = "\n".join(map(lambda x: "            case Type::" + x.name + ": " + x.name + "= other." + x.name + ";", enum_items))
    ctors = "\n".join(map(lambda x: "    {enum_name}({type_name} const& x) : type(Type::{item_name}), {item_name}(x) {{}}".format(item_name=x.name, enum_name=enum_name, type_name=x.type), enum_items))

    return """
struct {enum_name} {{
    enum class Type : {types_type} {{
        none,
{types_str}
    }};

    Type type;

    union {{
{union_str}
    }};

    {enum_name}& operator=({enum_name} const& other) {{
        switch(other.type) {{
{switch_assign_op}
        }}
        return *this;
    }}

    {enum_name}() : type(Type::none) {{
        memset(this, 0, sizeof({enum_name}));
    }}
    {enum_name}({enum_name} const& other) {{
        switch(other.type) {{
{switch_assign_op}
        }}
    }}
    ~{enum_name}() noexcept {{}}

{ctors}
}};
""".format(enum_name=enum_name, types_type=types_type, types_str=types_str, union_str=union_str, switch_assign_op=switch_assign_op, ctors=ctors)

def process_generate_type_enums(next_macro: MacroDef, file_info: FileInfo):
    [enum_name, next_ind] = extract_word(next_macro.macro_cmd)
    enum_items_str = extract_args_from_braces(next_macro.macro_cmd[next_ind:], "{", "}")
    result_items = []

    class ResultItem:
        def __init__(self, item_name: str, item_type: str):
            self.name = item_name
            self.type = item_type

    for ei in enum_items_str:
        [item_name, next_ind] = extract_word(ei, delim=[ "(", ")" ])
        item_type = substr_from_braces(ei[next_ind:], "(", ")")
        result_items.append(ResultItem(item_name=item_name, item_type=item_type))
        print(enum_name, item_name, item_type)
    return generate_enum_code(enum_name, result_items)

# ----------------------------------------    ENUM MACRO    ------------------------------------------------

def process_generate_enum_to_string(next_macro: MacroDef, file_info: FileInfo):
    [enum_name, next_ind] = extract_word(next_macro.macro_cmd)
    enum_items_str = extract_args_from_braces(next_macro.macro_cmd[next_ind:], "(", ")")
    return "switch (x) {{ {} }}\nreturn \"\";".format("".join(map(lambda x: "\ncase {enum_name}::{x}: return \"{x}\";".format(enum_name=enum_name, x=x), enum_items_str)))

def process_generate_enum_from_string(next_macro: MacroDef, file_info: FileInfo):
    [enum_name, next_ind] = extract_word(next_macro.macro_cmd)
    enum_items_str = extract_args_from_braces(next_macro.macro_cmd[next_ind:], "(", ")")
    return "".join(map(lambda x: "if (str == \"{x}\") {{ out = {enum_name}::{x}; return true; }}\n".format(enum_name=enum_name, x=x), enum_items_str)) + "return false;"

# ----------------------------------------     MACRO PROCESSOR     ----------------------------------------------------------------------

def run_macro_on_file(macro_name: str, macro_func: Callable[[MacroDef, FileInfo], str], file_info: FileInfo):
    has_smth = False
    next_macro: MacroDef or None = find_macro_def(file_info.file_source, macro_name=macro_name)
    while (next_macro != None):
        has_smth = True
        out_str = macro_func(next_macro, file_info)
        [new_source, next_ind] = next_macro.insert_result(file_info.file_source, out_str)
        file_info.file_source = new_source
        next_macro = find_macro_def(file_info.file_source, macro_name=macro_name, from_index=next_ind)
    return has_smth

def process_file(macros: "dict[str, Callable[[MacroDef, FileInfo], str]]", file_rel_path: str, file_full_path: str):
    with open(file_full_path, "rt") as file:
        file_source = file.read()
    file_info = FileInfo(file_rel_path=file_rel_path, file_full_path=file_full_path, file_source=file_source)
    has_changes = False

    for macro_name in macros:
        macro_func = macros[macro_name]
        is_done_smth = run_macro_on_file(macro_name=macro_name, macro_func=macro_func, file_info=file_info)
        has_changes = is_done_smth or is_done_smth
        if (is_done_smth):
            print(macro_name, " made changes in ", file_rel_path)
    
    if (has_changes):
        with open(file_full_path, "wt") as file:
            file.write(file_info.file_source)
            file.flush()
        files_cache[file_rel_path] = os.stat(file_full_path).st_mtime

# -----------------------------------------------------------------------------------------------------------------------------------------------

macros = {
    "_generate_type_enum": process_generate_type_enums,
    "_generate_enum_to_string": process_generate_enum_to_string,
    "_generate_enum_from_string": process_generate_enum_from_string
}

# get cache
if (os.path.exists(CACHE_FILE_PATH)):
    with open(CACHE_FILE_PATH, "rt") as cache_file:
        json_dec = json.decoder.JSONDecoder()
        files_cache = json_dec.decode(cache_file.read())

# search changed files
found_files = glob.glob(SRC_FILES_HPP, recursive=True) + glob.glob(SRC_FILES_CPP, recursive=True)
for file_path_ in found_files:
    file_full_path = os.path.realpath(file_path_).replace("\\", "/")
    file_rel_path = os.path.relpath(file_full_path, ROOT_DIR).replace("\\", "/")

    file_modify_time = os.stat(file_full_path).st_mtime
    cached_modify_time = files_cache.get(file_rel_path)

    if (cached_modify_time == None or file_modify_time > cached_modify_time):
        files_cache.setdefault(file_rel_path, file_modify_time)
        process_file(macros=macros, file_rel_path=file_rel_path, file_full_path=file_full_path)

# update cache
with open(CACHE_FILE_PATH, "wt") as cache_file:
    json_enc = json.encoder.JSONEncoder()
    files_cache_s = json_enc.encode(files_cache)
    cache_file.write(files_cache_s)
