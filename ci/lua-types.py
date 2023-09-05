import re
import xml.etree.ElementTree as ET
from collections.abc import Iterable
from dataclasses import dataclass
from enum import Enum as EnumType
from pathlib import Path
from typing import Any

########################################
#         Methods description          #
########################################

PATH_LUA_DOCS = "./docs/dev/Lua API.rst"

# PATTERN_LUA_DOCS = r"^\*\s``((.+)\(.*\))``\n^((\n|(\s+.*\n))+)"
PATTERN_LUA_DOCS = r"\*\s+``(?P<name1>[^`]+?)``(?:(?:, |\n\*\s+|\n  |, or )``(?P<namek>[^`]+?)``)*\n\n(?P<descr>(\n|(?!\*)[ \.]+[^\n]+)+)"

docs: dict[tuple[str, str], str] = {}


def parse_docs() -> dict[tuple[str, str], str]:
    print("Parsing Lua docs...")
    out: dict[tuple[str, str], str] = {}
    data = Path(PATH_LUA_DOCS).read_text()
    for match in re.finditer(PATTERN_LUA_DOCS, data, re.MULTILINE):
        for group in ["name1", "namek"]:
            if match.group(group):
                (class_name, fn_name, _with_self) = split_name(match.group(group))
                if fn_name:
                    out[(class_name.lower().strip(), fn_name.lower().strip())] = match.group(3)[:-1]
    print("Items -> total:", len(out))
    return out


def multiline_comment(comment: str, ending: str = "") -> str:
    return "--[=[" + comment.strip("--").strip("\n").replace("--", "") + ending + "]=]\n" if comment else ""


def split_name(value: str) -> tuple[str, str, bool]:
    for c in ["dfhack.", "dfhack:", "function", "=", "{", "}", "'", "?"]:
        value = value.replace(c, "")
    value = value.strip().split("(")[0]
    class_name = ""
    fn_name = value
    splitted = value.split(":")
    with_self = False
    if len(splitted) > 1:
        class_name = splitted[0]
        fn_name = splitted[1]
        with_self = True
    else:
        splitted = value.split(".")
        if len(splitted) > 1:
            class_name = splitted[0]
            fn_name = splitted[1]
    return (class_name, fn_name, with_self)


########################################
#        Signatures processing         #
########################################

PATH_LUAAPI = "./library/LuaApi.cpp"
PATH_LUATOOLS = "./library/LuaTools.cpp"
PATH_LIBRARY = "./library/"
PATH_DFHACK_OUTPUT = "./types/library/signatures.lua"
PATH_DEFAULT = "./types/library/default.lua"

PATTERN_WRAPM = r"WRAPM\((.+),[ ]*(.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_CWRAP = r"CWRAP\((.+),[ ]*(.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_WRAPN = r"WRAPN\((.+), (.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_WRAP = r"WRAP\((.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_MODULE_ARRAY = r"dfhack_[\w_]*module\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ARRAY = r"dfhack_[\w_]*funcs\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ITEM = r"\{\s\"(\w+)\",\s(\w+)\s\}[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_SIGNATURE = r"^([\w:<>]+)(.+)?\(([^)]*)\)"
PATTERN_SIGNATURE_SEARCH = r"^.+[\s\*]+(DFHack::){0,1}module::function[\s]{0,1}\([\n]{0,1}(.|,\n)*?\n{0,1}\)[\s\n\w]+\{"
PATTERN_FORCE_EXPOSE = r"\/\/\s?<force-expose>\s?(.+)"

BANNED_TYPES = ["lua_State", "color_ostream", "MapExtras::MapCache"]

MODULES_GLUE = """---@generic T
---@param module `T`
---@return T | _G | _global
function mkmodule(module) end

---@generic T
---@param module `T`
---@return T
function require(module) end

-- Create or updates a class; a class has metamethods and thus own metatable.
---@param class any
---@param parent any
---@return any
function defclass(class, parent) end
"""


@dataclass
class Arg:
    name: str
    type: str
    default_value: Any | None
    vararg: bool
    unknown: bool = False


@dataclass
class Ret:
    type: str
    unknown: bool = False


@dataclass
class Signature:
    ret: Ret
    args: list[Arg]
    name: str | None


@dataclass
class Entry:
    module: str
    fn: str
    type: str
    sig: str | None
    decoded_sig: Signature | None
    ignore_expose_prefix: bool = False


def parse_files() -> Iterable[Entry]:
    total = 0
    found = 0
    decoded = 0
    for path in [PATH_LUAAPI, PATH_LUATOOLS]:
        data = Path(path).read_text(encoding="utf-8")
        arrays = [PATTERN_MODULE_ARRAY, PATTERN_LFUNC_ARRAY]
        wrappers = [WRAPM, WRAPN, CWRAP, LFUNC, WRAP]
        for array_pattern in arrays:
            for array in re.finditer(array_pattern, data):
                for wrapper in wrappers:
                    for item in wrapper(array.group(0)):
                        total += 1
                        if item.sig:
                            found += 1
                            item.decoded_sig = decode_signature(item.sig)
                            if item.decoded_sig:
                                decoded += 1
                            yield item
                        else:
                            print("Unable to find signature -> module:", item.module, "function:", item.fn)
        for match in re.finditer(PATTERN_FORCE_EXPOSE, data, re.MULTILINE):
            if match.group(1):
                total += 1
                found += 1
                decoded_signature = decode_signature(match.group(1).strip(), False)
                if decoded_signature:
                    decoded += 1
                    item = Entry(
                        module="",
                        fn=decoded_signature.name or "INVELID_NAME",
                        type="FORCE_EXPOSE",
                        sig=match.group(1).strip(),
                        decoded_sig=decoded_signature,
                        ignore_expose_prefix=True,
                    )
                    yield item

    print(f"Signatures -> total: {total}, found: {found}, decoded {decoded}")


def module_name(array: str) -> str:
    return (
        array.split(" ")[0]
        .replace("dfhack", "")
        .replace("module[]", "")
        .replace("funcs[]", "")
        .replace("_", "")
        .capitalize()
    )


def WRAP(array: str) -> Iterable[Entry]:
    module = module_name(array) or ""
    for match in re.finditer(PATTERN_WRAP, array):
        item = Entry(module, match.group(1).split(",")[0], "WRAP", None, None)
        if match.group(2):
            item.sig = match.group(2).split("<expose>")[1].strip()
        else:
            item.sig = find_signature(item)
        yield item


def LFUNC(array: str) -> Iterable[Entry]:
    module = module_name(array)
    for match in re.finditer(PATTERN_LFUNC_ITEM, array):
        item = Entry(module, match.group(1), "LFUNC", None, None)
        if match.group(3):
            item.sig = match.group(3).split("<expose>")[1].strip()
        else:
            item.sig = find_signature(item)
        yield item


def CWRAP(array: str) -> Iterable[Entry]:
    module = module_name(array)
    for match in re.finditer(PATTERN_CWRAP, array):
        item = Entry(module, match.group(1), "CWRAP", None, None)
        if match.group(3):
            item.sig = match.group(3).split("<expose>")[1].strip()
        else:
            item.sig = find_signature(item)
        yield item


def WRAPM(array: str) -> Iterable[Entry]:
    for match in re.finditer(PATTERN_WRAPM, array):
        if any(c in match.group(1) or c in match.group(2) for c in ["{", "}"]):
            continue
        item = Entry(match.group(1), match.group(2), "WRAPM", None, None)
        if match.group(3):
            item.sig = match.group(3).split("<expose>")[1].strip()
        else:
            item.sig = find_signature(item)
        yield item


def WRAPN(array: str) -> Iterable[Entry]:
    module = module_name(array)
    for match in re.finditer(PATTERN_WRAPN, array):
        if any(c in match.group(1) or c in match.group(2) for c in ["{", "}"]):
            continue
        item = Entry(module, match.group(1), "WRAPN", None, None)
        if match.group(3):
            item.sig = match.group(3).split("<expose>")[1].strip()
        else:
            item.sig = find_signature(item)
        yield item


def find_signature(item: Entry) -> str | None:
    for entry in Path(PATH_LIBRARY).rglob("*.cpp"):
        data = entry.read_text(encoding="utf-8")
        regex: set[str] = set()
        if item.module != "":
            regex.add(PATTERN_SIGNATURE_SEARCH.replace("module", item.module).replace("function", item.fn))
        regex.add(PATTERN_SIGNATURE_SEARCH.replace("module::", "").replace("function", item.fn))
        for r in regex:
            for match in re.finditer(r, data, re.MULTILINE):
                sig = match.group(0).replace("\n", "").replace("{", "").replace("DFHACK_EXPORT ", "").strip()
                if sig.startswith("if (") or sig.startswith("<<") or sig.find("&&") > 0 or sig.find("->") > 0:
                    continue
                return sig
    return None


def decode_type(cxx: str) -> str:
    splitted = cxx.strip().split("|")
    out: list[str] = []
    for s in splitted:
        out.append(decode_single_type(s.strip()))
    return "|".join(out)


def decode_single_type(cxx: str) -> str:
    match cxx:
        case "int" | "int8_t" | "uint8_t" | "int16_t" | "uint16_t" | "int32_t" | "uint32_t" | "int64_t" | "uint64_t" | "size_t" | "uintptr_t" | "intptr_t":
            return "integer"
        case "float" | "long" | "ulong" | "double":
            return "number"
        case "bool":
            return "boolean"
        case "string" | "std::string" | "char":
            return "string"
        case "void":
            return "nil"
        case _:
            if cxx.startswith("std::function"):
                return "function"
            if cxx.startswith("std::vector<") or cxx.startswith("vector<"):
                return decode_type(cxx.replace("std::vector<", "").replace("vector<", "")[:-1]).strip() + "[]"
            if cxx.startswith("df::"):
                return cxx[4:]
            if cxx.startswith("std::unique_ptr<"):
                return decode_type(cxx.replace("std::unique_ptr<", "")[:-1]).strip()
            return cxx


def decode_signature(sig: str, lower: bool = True) -> Signature | None:
    for k in ["const ", "*", "&", "static ", "inline "]:
        sig = sig.replace(k, "")
    match = re.search(PATTERN_SIGNATURE, sig, re.MULTILINE)
    if match:
        type_ret = match.group(1)
        decoded_type_ret = decode_type(type_ret)
        args_pairs = match.group(3).split(", ")
        args: list[Arg] = []
        if args_pairs.__len__() > 0 and match.group(3).__len__() > 0:
            for arg_pair in args_pairs:
                vararg = False
                default_value = None
                is_default = arg_pair.split("=")
                if is_default.__len__() > 1:
                    default_value = is_default[1].strip()
                    arg_pair = is_default[0].strip()
                if arg_pair.startswith("..."):
                    vararg = True
                    arg_name = "vararg"
                    arg_type = "vararg_type"
                    decoded_type_arg = "..."
                else:
                    arg_name = arg_pair.split(" ")[-1]
                    arg_type = arg_pair.replace(" " + arg_name, "").strip()
                    if arg_type in BANNED_TYPES:
                        continue
                    decoded_type_arg = decode_type(arg_type)
                arg = Arg(
                    name=arg_name,
                    type=decoded_type_arg.replace("::", "."),
                    default_value=default_value,
                    unknown=(decoded_type_arg == arg_type and arg_type != "string")
                    or any(c.isupper() for c in decoded_type_arg),
                    vararg=vararg,
                )
                args.append(arg)
        if decoded_type_ret:
            name = match.group(2).strip().replace("::", ".") if match.group(2) else None
            if lower and name:
                name = name.lower()
            return Signature(
                ret=Ret(
                    type=decoded_type_ret.replace("::", ".").replace("enums__biome_type__", ""),
                    unknown=decoded_type_ret == type_ret and type_ret != "string",
                ),
                args=check_optional_bool(args),
                name=name,
            )
    return None


def check_optional_bool(args: list[Arg]) -> list[Arg]:
    for arg in reversed(args):
        if arg.type == "boolean":
            arg.default_value = "false"
        else:
            break
    return args


def print_entry(entry: Entry, prefix: str = "dfhack.") -> str:
    if entry.ignore_expose_prefix:
        prefix = ""
    sig = " ".join(entry.sig.split()) if entry.sig else entry.sig
    s = ""
    if (entry.module.lower(), entry.fn.lower()) in docs:
        s += multiline_comment(docs[(entry.module.lower(), entry.fn.lower())], ending="\n")
    s += f"-- CXX SIGNATURE -> `{sig}`\n"
    known_args = ""
    ret = ""
    if entry.decoded_sig:
        for arg in entry.decoded_sig.args:
            if arg.vararg:
                known_args += f"---@vararg unknown\n"
                break
            else:
                known_args += f"---@param {arg.name}{'?' if arg.default_value else ''} {arg.type}{' -- unknown' if arg.unknown else ''}{' -- default value is ' + arg.default_value if arg.default_value else ''}\n"
        ret = f"---@return {entry.decoded_sig.ret.type}{' -- unknown' if entry.decoded_sig.ret.unknown else ''}\n"
        s += known_args + ret
        args = f"{', '.join(['...' if x.vararg else x.name for x in entry.decoded_sig.args])}"
        s += f"function {prefix}{entry.module.lower()}{'.' if entry.module != '' else ''}{entry.fn}({args}) end\n"
    return s


dfhack_modules: set[str] = set()


def generate_base_methods() -> None:
    with Path(PATH_DEFAULT).open("w", encoding="utf-8") as dest:
        out = f"-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n---@class df\ndf = nil\n\n---@class global\ndf.global = nil\n\n---@class dfhack\ndfhack = nil\n"
        for module in dfhack_modules:
            out += f"\n---@class {module.lower()}\ndfhack.{module.lower()} = nil\n"
        out += "\n" + MODULES_GLUE
        print(out, file=dest)


def signatures_processing() -> None:
    print("Processing signatures...")
    with Path(PATH_DFHACK_OUTPUT).open("w", encoding="utf-8") as dest:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=dest)
        unknown_types: set[str] = set()
        for entry in parse_files():
            if entry.decoded_sig:
                if not entry.ignore_expose_prefix and entry.module:
                    dfhack_modules.add(entry.module)
                if entry.ignore_expose_prefix and entry.fn.startswith("dfhack.") and entry.fn.split(".").__len__() > 2:
                    dfhack_modules.add(entry.fn.split(".")[1])
                if entry.decoded_sig.ret.unknown:
                    unknown_types.add(f"--@alias {entry.decoded_sig.ret.type.replace('[]', '')} unknown\n")
                for arg in entry.decoded_sig.args:
                    if arg.unknown:
                        unknown_types.add(f"--@alias {arg.type.replace('[]', '')} unknown\n")
            print(print_entry(entry), file=dest)
        if unknown_types.__len__() > 0:
            print(f"\n-- Unknown types\n{''.join(unknown_types)}", file=dest)

    generate_base_methods()


########################################
#          Symbols processing          #
########################################

PATH_CODEGEN = "./library/include/df/codegen.out.xml"
PATH_DEFINITIONS = "./types/library/symbols.lua"

PATH_XML = "./library/xml"
PATH_OUTPUT = "./types/library/"
PATH_LIB_CONFIG = "./types/config.json"
PATH_CONFIG = ".luarc.json"

LIB_CONFIG = """{
  "name": "DFHack Lua",
  "words": [
      "dfhack"
  ]
}\n"""

CONFIG = f"""{{
  "workspace.library": ["{PATH_OUTPUT}"],
  "workspace.ignoreDir": ["build"],
  "workspace.useGitIgnore": false,
  "diagnostics.disable": ["lowercase-global"]
}}\n"""


BASE_METHODS = """---@field sizeof fun(self: self): integer
---@field new fun(self: self): self
"""

BASE_TYPED_OBJECTS_METHODS = """---@field assign fun(self: self, obj: self): nil
---@field delete fun(self: self): boolean
---@field _displace fun(self: self, index: integer): self
---@field _kind "primitive"|"struct"|"container"|"bitfield"
---@field _type table | string
"""

BASE_NAMED_TYPES_METHODS = """---@field is_instance fun(self: self, obj: any): boolean
---@field _identity lightuserdata
---@field _kind "struct-type"|"class-type"|"enum-type"|"bitfield-type"|"global"
---@field _fields table<string, { name: string, offset: integer, count: integer, mode: any, type_name?: string, type?: any, type_identity?: lightuserdata, index_enum?: any, union_tag_field?: string, union_tag_attr?: string, original_name?: string }>
"""

BASE_STRUCT_METHODS = """---@field vmethod fun(self: self, ...): any
---@field is_instance fun(self: self, obj: any): boolean
"""

BASE_CONTAINER_METHODS = """---@field resize fun(self: self, size: integer): nil
---@field insert fun(self: self, index: "#"|integer, item: <UNDERLAYING>): nil
---@field erase fun(self: self, index: integer): nil
"""

df_global: list[str] = []
df: list[str] = []


class RenderMode(EnumType):
    Type = "type"
    Regular = "regular"
    All = "all"


class Tag:
    renderable = False
    fieldable = False
    el: ET.Element
    tag: str
    meta: str
    level: str
    name: str
    type_name: str
    typedef_name: str
    anon_name: str
    subtype: str
    ref_target: str
    pointer_type: str
    instance_vector: str
    union: str
    inherit: str
    comment: str
    since: str
    path: list[str]
    full_type: str
    name_prefix: str = ""
    multiline_comment: str = ""
    render_mode: RenderMode
    have_childs: bool = False
    is_container: bool = False

    def __init__(self, el: ET.Element, render_mode: RenderMode, path: list[str] = []) -> None:
        self.el = el
        self.path = path
        self.render_mode = render_mode
        self.parse()
        self.fetch()

    def parse(self) -> None:
        self.have_childs = len(self.el) > 0
        if not self.have_childs:
            self.renderable = False
        if len(self.el) == 1 and self.el[0].tag == "item":
            self.el.attrib = self.el[0].attrib | self.el.attrib
            self.renderable = False

        self.tag = self.el.tag
        self.meta = self.el.attrib.get("meta") or ""
        self.level = self.el.attrib.get("level") or ""
        self.name = self.el.attrib.get("name") or ""
        self.type_name = self.el.attrib.get("type-name") or ""
        self.typedef_name = self.el.attrib.get("typedef-name") or ""
        self.anon_name = self.el.attrib.get("anon-name") or ""
        self.subtype = self.el.attrib.get("subtype") or ""
        self.ref_target = self.el.attrib.get("ref-target") or ""
        self.pointer_type = self.el.attrib.get("pointer-type") or ""
        self.instance_vector = self.el.attrib.get("instance-vector") or ""
        self.union = self.el.attrib.get("is-union") or ""
        self.inherit = self.el.attrib.get("inherits-from") or ""
        self.since = self.el.attrib.get("since") or ""
        self.comment = self.el.attrib.get("comment") or ""
        if self.comment:
            self.comment = " " + self.comment
        if self.since:
            self.comment += " since " + self.since

        if (RenderMode.Regular or RenderMode.All) and (self.name or self.anon_name):
            self.fieldable = True
        if RenderMode.Type and self.typedef_name:
            self.fieldable = True

        # case for root global-type
        if self.level == "0":
            self.path.append(self.type_name)
        else:
            if self.render_mode == RenderMode.Type:
                self.path.append(self.typedef_name)
            else:
                self.path.append(self.name)
        if self.meta == "global":
            self.path = [self.path[-1]]
            self.full_type = self.type_name
        else:
            self.full_type = ".".join(self.path)

        if self.tag == "global-object":
            self.path = [self.name]
            self.full_type = ".".join(self.path)

        if self.full_type in df_global and "global" not in self.path:
            self.path.insert(0, "global")
            self.full_type = ".".join(self.path)

    def fetch(self) -> None:
        pass

    def as_field(self) -> str:
        return "abstract as_field"

    def render(self) -> str:
        return "abstract render"


class EnumItem(Tag):
    renderable = False
    index: int

    def set_index(self, index: int):
        self.index = index
        return self

    def as_field(self) -> str:
        name = self.name or f"unk_{self.index}"
        return f"{' ' * 4}{name} = {self.index},{' --' + self.comment if self.comment else ''}"


class Enum(Tag):
    renderable = True
    items: list[EnumItem]
    attrs: list[tuple[str, str | None]]

    def fetch(self) -> None:
        self.attrs = []
        self.items = []
        shift = 0
        for i, child in enumerate(self.el, start=0):
            if child.tag == "enum-item":
                if "value" in child.attrib:
                    shift = int(child.attrib["value"]) - i
                self.items.append(EnumItem(child, self.render_mode).set_index(i + shift))
            else:
                if child.tag == "enum-attr":
                    name = child.attrib.get("name")
                    if name:
                        self.attrs.append((name, child.attrib.get("type-name")))
                shift -= 1

        enum_map[self.full_type] = self

    def render(self) -> str:
        s = f"-- {self.render_mode}, path: {self.path}, enum\n"
        s += f"---@enum {self.full_type}{self.comment}\n"
        s += f"df.{self.full_type} = {{\n"
        for item in self.items:
            s += f"{item.as_field()}\n"
        s += "}\n\n"
        if len(self.attrs) > 0:
            s += f"---@type {{ [{self.full_type}]: {{ "
            for attr in self.attrs:
                s += f"{attr[0]}: {base_type(attr[1] or 'unknown')}, "
            s = s[:-2] + f" }}}}\ndf.{self.full_type}.attrs = nil\n"
        return s

    def as_field(self) -> str:
        return f"---@field {self.path[-1]} {self.full_type}{self.comment}"

    def keys(self) -> list[str]:
        keys: list[str] = []
        for item in self.items:
            keys.append(item.name or f"unk_{item.index}")
        return keys


enum_map: dict[str, Enum] = {}


class MultilineComment(Tag):
    renderable = False
    fieldable = False

    def fetch(self) -> None:
        self.multiline_comment = self.el.text or ""
        self.multiline_comment = self.multiline_comment.replace("    ", "")


class Primitive(Tag):
    renderable = False

    def as_field(self) -> str:
        return f"---@field {self.name or self.anon_name} {base_type(self.subtype)}{self.comment}"


class IterableTag(Tag):
    items: list[Tag]

    def underlaing_entity(self) -> None:
        (self.inside_el, self.array_dim) = get_container_item(self.el)
        self.postfix_brackets = "[]" * len(self.array_dim)
        if self.render_mode == RenderMode.Type and not self.typedef_name and "typedef-name" in self.inside_el.attrib:
            self.typedef_name = self.inside_el.attrib["typedef-name"]
            self.path = self.path[:-1]
            self.path.append(self.typedef_name)
            self.full_type = ".".join(self.path)

    def fetch_items(self) -> None:
        if self.have_childs and len(self.inside_el) > 0:
            if self.inside_el.attrib.get("name") and self.el.attrib.get("name"):
                self.inside_el.attrib["name"] = self.el.attrib["name"]
            self.inside_el.attrib = self.el.attrib | self.inside_el.attrib
            if self.inside_el.attrib.get("subtype") == "enum":
                item = Enum(self.inside_el, self.render_mode, self.path[:-1])
            else:
                item = Compound(self.inside_el, self.render_mode, self.path[:-1])
            item.renderable = True
            self.items.append(item)

    def unknown_vector_item(self) -> None:
        if (
            len(self.array_dim) > 0
            and self.inside_el.attrib.get("meta") == "container"
            and self.inside_el.attrib["subtype"] == "stl-vector"
        ):
            self.typed = "unknown" + self.postfix_brackets

    def primitive_item(self) -> None:
        if self.inside_el.attrib.get("meta") == "global":
            self.typed = self.inside_el.attrib["type-name"] + self.postfix_brackets
        if self.inside_el.attrib.get("meta") == "primitive" or self.inside_el.attrib.get("meta") == "number":
            self.typed = base_type(self.inside_el.attrib["subtype"]) + self.postfix_brackets

    def index_enum(self, value_type: str) -> None:
        if "index-enum" in self.el.attrib and self.el.attrib["index-enum"] in enum_map:
            self.typed = "table<"
            for key in enum_map[self.el.attrib["index-enum"]].keys():
                self.typed += f'"{key}"|'
            self.typed = f"{self.typed[:-1]}, {value_type}>"
        else:
            self.typed = f"table<string, {value_type}>"


class Pointer(IterableTag):
    renderable = False
    items: list[Tag]

    def fetch(self) -> None:
        self.items = []
        self.underlaing_entity()
        self.fetch_items()
        if self.have_childs and len(self.inside_el) > 0:
            self.renderable = True

        if len(self.items) > 0:
            type_name = f"{self.full_type}"
        elif self.pointer_type and self.subtype == "stl-vector":
            type_name = self.pointer_type + "[]"
        else:
            type_name = ""
        subtype = "any[] -- NOT TYPED" if base_type(self.subtype) == "stl-vector" else base_type(self.subtype)
        self.typed = base_type(self.type_name) or type_name or subtype or self.full_type

        self.primitive_item()
        self.unknown_vector_item()

        if self.typed == self.full_type:
            if self.render_mode == RenderMode.Type:
                self.renderable = True
            if not self.have_childs and not self.type_name:
                self.typed = "unknown -- NOT TYPED"

    def as_field(self) -> str:
        count = f"count<{','.join(self.array_dim)}>"
        return f"---@field {self.path[-1]} {self.typed}{' ' + count if len(self.array_dim) > 0 else ''}{self.comment}"

    def render(self) -> str:
        s = ""
        if self.typed == self.full_type and self.render_mode == RenderMode.Type:
            s += f"-- {self.render_mode}, path: {self.path}, pointer\n"
            s += f"---@class {self.full_type}{self.comment}\n"
            s += f"df.{self.full_type} = nil\n"
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class Container(IterableTag):
    renderable = True
    is_container = True
    items: list[Tag]

    def fetch(self) -> None:
        self.renderable = True
        self.items = []
        self.underlaing_entity()
        self.fetch_items()

        inside_containter = self.full_type if len(self.inside_el) > 0 and self.inside_el[0].tag != "code-helper" else ""
        self.typed = f"{self.ref_target or base_type(self.type_name) or inside_containter or ('boolean' if self.subtype == 'stl-bit-vector' else '') or 'any'}{'[]' * len(self.array_dim)}"

        if self.subtype == "df-flagarray":
            self.index_enum("boolean")

        self.primitive_item()
        self.unknown_vector_item()

        if self.inside_el.attrib.get("subtype") == "stl-string" and self.pointer_type == "stl-string":
            self.typed = f"{{ value: string }}{self.postfix_brackets}"

        if self.typed.startswith(self.full_type + "[]"):
            if self.render_mode == RenderMode.Type:
                self.renderable = True
            if not self.have_childs and not self.type_name:
                self.typed = "unknown -- NOT TYPED"

    def as_field(self) -> str:
        count = f" count<{','.join(self.array_dim)}>"
        type_name = self.full_type + "_C"
        if self.subtype == "df-flagarray":
            count = ""
            type_name = self.typed
        return f"---@field {self.path[-1]} {type_name}{count}{self.comment}{' -- NOT TYPED' if self.typed == 'any[]' else ''}"

    def as_type(self) -> str:
        count = f" count<{','.join(self.array_dim)}>"
        if self.subtype == "df-flagarray":
            count = ""
        return f"---@type {self.typed}{count}{self.comment}{' -- NOT TYPED' if self.typed == 'any[]' else ''}"

    def render(self) -> str:
        s = ""
        if self.subtype != "df-flagarray":
            s += f"-- {self.render_mode}, path: {self.path}, container\n"
            s += f"---@class {self.full_type}_C{(': ' + self.inherit) if self.inherit else ''}{self.comment}\n"
            s += f"---@field [integer] {self.typed[:-2] if self.typed.endswith('[]') else self.typed}\n"
            s += BASE_CONTAINER_METHODS.replace(
                "<UNDERLAYING>", self.typed[:-2] if self.typed.endswith("[]") else self.typed
            )
            if self.render_mode == RenderMode.Type:
                s += BASE_NAMED_TYPES_METHODS
            else:
                s += BASE_TYPED_OBJECTS_METHODS
            s += f"df.{self.full_type}_C = nil\n\n"
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class StaticArray(IterableTag):
    renderable = False
    is_container = True
    items: list[Tag]

    def fetch(self) -> None:
        self.items = []
        self.items = []
        self.underlaing_entity()
        self.fetch_items()
        if self.have_childs and len(self.inside_el) > 0:
            self.renderable = True

        inside_containter = self.full_type if len(self.inside_el) > 0 and self.inside_el[0].tag != "code-helper" else ""
        self.typed = (
            f"{self.ref_target or base_type(self.type_name) or inside_containter or 'any'}{self.postfix_brackets}"
        )

        if "index-enum" in self.el.attrib:
            self.index_enum(self.typed[:-2] if self.typed.endswith("[]") else self.typed)
        else:
            item_type = (
                self.inside_el.attrib.get("type-name")
                or self.inside_el.attrib.get("base-name")
                or self.inside_el.attrib.get("subtype")
                or "any"
            )
            match self.inside_el.attrib.get("meta"):
                case "global" | "number" | "primitive":
                    self.typed = base_type(item_type) + self.postfix_brackets
                case "compound":
                    pass
                case _:
                    match self.inside_el.attrib.get("subtype"):
                        case "bitfield" | "enum":
                            self.typed = self.full_type + self.postfix_brackets
                        case _:
                            self.typed = base_type(item_type) + self.postfix_brackets

        self.unknown_vector_item()

    def as_field(self) -> str:
        return f"---@field {self.path[-1]} {self.typed} count<{','.join(self.array_dim)}>{self.comment}{' -- NOT TYPED' if self.typed == 'any[]' else ''}"

    def as_type(self) -> str:
        return f"---@type {self.typed} count<{','.join(self.array_dim)}>{self.comment}{' -- NOT TYPED' if self.typed == 'any[]' else ''}"

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class Compound(Tag):
    renderable = True
    items: list[Tag]
    has_container = False

    def fetch(self) -> None:
        self.items = []
        for i, child in enumerate(self.el):
            for tag in switch_tag(child, self.render_mode, self.path[:]):
                if not tag.name and not tag.anon_name:
                    tag.name = f"unnamed_{i}"
                if (self.render_mode == RenderMode.Type and tag.typedef_name) or (
                    self.render_mode == RenderMode.Regular and tag.name
                ):
                    if tag.is_container:
                        self.has_container = True
                    if tag.multiline_comment:
                        self.multiline_comment = tag.multiline_comment
                        continue
                    self.items.append(tag)

    def as_field(self) -> str:
        return f"---@field {self.path[-1]} {self.full_type}{self.comment}"

    def as_type(self) -> str:
        return f"---@type {self.full_type}{self.comment}"

    def render(self) -> str:
        s = ""
        if self.multiline_comment:
            s += f"--[[{self.multiline_comment}\n]]\n"
        s += f"-- {self.render_mode}, path: {self.path}, compound\n"
        s += f"---@class {self.full_type}{(': ' + self.inherit) if self.inherit else ''}{self.comment}\n"
        append = ""
        for item in self.items:
            if item.fieldable:
                s += f"{item.as_field()}\n"
            if item.renderable:
                append += item.render()
        if not self.inherit:
            s += BASE_METHODS
            if self.render_mode == RenderMode.Type:
                s += BASE_NAMED_TYPES_METHODS
            else:
                s += BASE_TYPED_OBJECTS_METHODS
        s += f"df.{self.full_type} = nil\n"
        return s + "\n" + append


class Struct(Tag):
    renderable = True
    items: list[Tag]
    has_container = False

    def fetch(self) -> None:
        self.items = []
        for mode in [RenderMode.Regular, RenderMode.Type]:
            for i, child in enumerate(self.el):
                for tag in switch_tag(child, mode, self.path[:]):
                    if not tag.name and not tag.anon_name:
                        tag.name = f"unnamed_{i}"
                    if (mode == RenderMode.Type and tag.typedef_name) or (mode == RenderMode.Regular and tag.name):
                        if tag.is_container:
                            self.has_container = True
                        if tag.multiline_comment:
                            self.multiline_comment = tag.multiline_comment
                            continue
                        self.items.append(tag)

    def render(self) -> str:
        s = ""
        if self.multiline_comment:
            s += f"--[[{self.multiline_comment}\n]]\n"
        s += f"-- {self.render_mode}, path: {self.path}, struct\n"
        s += f"---@class {self.full_type}{(': ' + self.inherit) if self.inherit else ''}{self.comment}\n"
        append = ""
        for item in self.items:
            if item.fieldable:
                s += f"{item.as_field()}\n"
            if item.renderable:
                append += item.render()
        if self.instance_vector:
            s += "---@field find fun(id: integer): self|nil\n"
        s += BASE_METHODS + BASE_STRUCT_METHODS + BASE_TYPED_OBJECTS_METHODS
        s += f"df.{self.full_type} = nil\n"
        return s + "\n" + append


class DefaultGlobalField(Tag):
    renderable = False

    def as_field(self) -> str:
        return f"---@field {self.name or self.anon_name} {self.type_name}{self.comment}"


class GlobalObject(Tag):
    renderable = True

    def render(self) -> str:
        match self.meta:
            case "global" | "pointer":
                prefix = "global." if self.type_name in df_global else ""
                return f" -- {self.render_mode}, path {self.path}\n---@type {prefix}{self.type_name}{self.comment}\ndf.{self.full_type} = nil\n"
            case "number":
                return f" -- {self.render_mode}, path {self.path}\n---@type {base_type(self.subtype)}{self.comment}\ndf.{self.full_type} = nil\n"
            case "static-array":
                tag = StaticArray(self.el[0], self.render_mode, self.path[:])
                return f" -- {self.render_mode}, path {self.path}\n{tag.as_type()}\ndf.{self.full_type} = nil\n"
            case "compound":
                self.el[0].attrib = self.el.attrib | self.el[0].attrib
                tag = Compound(self.el[0], self.render_mode, self.path[:])
                tag.path = tag.path[:-1]
                tag.full_type = ".".join(tag.path)
                return tag.render()
            case "container":
                tag = Container(self.el[0], self.render_mode, self.path[:])
                return f" -- {self.render_mode}, path {self.path}\n{tag.as_type()}\ndf.{self.full_type} = nil\n"
            case _:
                return "-- NOT HANDLED GLOBAL OBJECT " + self.name


def base_type(type: str) -> str:
    match type:
        case "int8_t" | "uint8_t" | "int16_t" | "uint16_t" | "int32_t" | "uint32_t" | "int64_t" | "uint64_t" | "size_t":
            return "integer"
        case "stl-string" | "static-string" | "ptr-string":
            return "string"
        case "s-float" | "d-float" | "long" | "ulong":
            return "number"
        case "bool" | "flag-bit":
            return "boolean"
        case "pointer" | "padding":
            return "integer"  # or what?
        case "stl-bit-vector" | "df-flagarray":
            return "boolean[]"
        case _:
            return type


def get_container_item(el: ET.Element) -> tuple[ET.Element, list[str]]:
    array_dim: list[str] = []
    while len(el) > 0:
        if (
            "is-array" in el.attrib
            or el.attrib.get("meta") == "static-array"
            or (el.attrib.get("subtype") == "stl-vector" and el.attrib.get("meta") == "container")
        ):
            array_dim.append(el.attrib.get("count") or "_")
        if el[0].tag != "item":
            return (el, array_dim)
        el = el[0]
    return (el, array_dim)


def switch_tag(el: ET.Element, render_mode: RenderMode, path: list[str] = []) -> Iterable[Tag]:
    if el.tag == "item" or el.tag == "code-helper":
        return
    match el.attrib.get("meta"):
        case "primitive" | "bytes" | "number":
            yield Primitive(el, render_mode, path)
        case "pointer":
            yield Pointer(el, render_mode, path)
        case "container":
            yield Container(el, render_mode, path)
        case "static-array":
            yield StaticArray(el, render_mode, path)
        case "compound":
            match el.attrib.get("subtype"):
                case "enum":
                    yield Enum(el, render_mode, path)
                case _:
                    # if tag.union == "true":
                    #     yield Union(el, render_mode, path)
                    # else:
                    if "anon-compound" in el.attrib:
                        el.attrib["name"] = "anon_compound"
                    yield Compound(el, render_mode, path)
        case "global":
            match el.attrib.get("subtype"):
                case "enum":
                    yield Enum(el, render_mode, path)
                case _:
                    yield DefaultGlobalField(el, render_mode, path)
            pass
        case _:
            if el.tag == "comment":
                yield MultilineComment(el, render_mode, path)
            else:
                # if el.tag == "virtual-methods":
                #     yield VirtualMethods(el, path, render_mode)
                # else:
                print("Skip tag ->", el.tag, path)


def switch_global_tag(el: ET.Element, only_enums: bool) -> Iterable[Tag]:
    if only_enums:
        if el.attrib.get("meta") == "enum-type":
            yield Enum(el, RenderMode.Regular, [])
    else:
        match el.attrib.get("meta"):
            case "struct-type" | "class-type" | "bitfield-type":
                yield Struct(el, RenderMode.All, [])
            case _:
                match el.tag:
                    case "global-object":
                        yield GlobalObject(el, RenderMode.Regular, [])
                    case _:
                        pass
                        # print(f"-- SKIPPED TAG {el.tag} {el.attrib.get('meta')} {el.attrib.get('type-name')}")


def parse_codegen(file: Path):
    tree = ET.parse(file)
    root = tree.getroot()
    total = 0
    with Path(PATH_DEFINITIONS).open("w", encoding="utf-8") as output:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=output)
        for enums in [True, False]:
            for el in root:
                for tag in switch_global_tag(el, enums):
                    total += 1
                    print(tag.render(), file=output)
    print(f"Symbol tags -> total: {total}")


def parse_items_place() -> None:
    for file in sorted(Path(PATH_XML).glob("*.xml")):
        data = file.read_text(encoding="utf-8")
        for match in re.finditer(r"(struct|class|enum)-type.*type-name='([^']+).*'", data, re.MULTILINE):
            df.append(match.group(2))
        for match in re.finditer(r"(global-object).*\sname='([^']+).*'", data, re.MULTILINE):
            df_global.append(match.group(2))


def symbols_processing() -> None:
    if not Path(PATH_OUTPUT).is_dir():
        Path(PATH_OUTPUT).mkdir(parents=True, exist_ok=True)
        Path(PATH_LIB_CONFIG).write_text(LIB_CONFIG, encoding="utf-8")
        Path(PATH_CONFIG).write_text(CONFIG, encoding="utf-8")
    tmp = Path(PATH_CODEGEN + ".tmp")
    data = Path(PATH_CODEGEN).read_text(encoding="utf-8")
    data = data.replace("ld:", "")
    tmp.write_text(data, encoding="utf-8")
    parse_items_place()
    parse_codegen(tmp)
    tmp.unlink()


########################################
#        Lua modules processing        #
########################################


PATH_LUA_MODULES = ["./library/lua", "./plugins/lua"]
PATH_LUA_MODULES_OUTPUT = "./types/library/"

PATTERN_MKMODULE = r"^local\s_ENV\s=\smkmodule\(['\"](.+)['\"]\)\n"
PATTERN_LUA_FUNCTION = r"((^--\s.+\n)*){0,1}((^---@.+\n)*){0,1}^function\s([\w:]+)\((.*)\)"
PATTERN_LUA_VARAIBLE = r"((^--\s.+\n)*){0,1}^(\w+)\s=.*\n"


@dataclass
class LuaFunc:
    fn_name: str
    class_name: str
    ret: str
    args: list[str]
    comment: str


def lua_modules_processing() -> None:
    print("Lua modules processing...")
    total = 0
    with Path(PATH_LUA_MODULES_OUTPUT + "modules.lua").open("w", encoding="utf-8") as dest:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=dest)
        for folder in PATH_LUA_MODULES:
            for entry in Path(folder).rglob("*.lua"):
                for item in parse_lua_file(entry):
                    total += 1
                    print(item, file=dest)
    print(f"Modules -> total: {total}")


def parse_lua_file(src: Path) -> Iterable[str]:
    data = src.read_text(encoding="utf-8")
    m = re.search(PATTERN_MKMODULE, data, re.MULTILINE)
    if not m:
        yield from parse_lua_module(data, "_global")
    else:
        yield from parse_lua_module(data, m.group(1))


def parse_lua_module(data: str, module: str, header: bool = True) -> Iterable[str]:
    classes: dict[str, list[str]] = {}
    for item in parse_lua_functions(data, module):
        target = item.class_name or module
        if target not in classes:
            classes[target] = []
        classes[target].append(item.comment + f"---@field {item.fn_name} fun({', '.join(item.args)}): {item.ret}\n")
    for match in re.finditer(PATTERN_LUA_VARAIBLE, data, re.MULTILINE):
        type_name = match.group(3) if match.group(3) in classes else "any"
        if module not in classes:
            classes[module] = []
        classes[module].append(multiline_comment(match.group(1)) + f"---@field {match.group(3)} {type_name}\n")
    for cl in classes:
        yield render_class((cl, classes[cl]), header)


def render_class(cl: tuple[str, list[str]], header: bool = True) -> str:
    s = ""
    if header:
        s += f"---@class {cl[0]}\n"
    for field in cl[1]:
        s += field
    return s


def parse_annotation(data: str) -> dict[str, str]:
    out: dict[str, str] = dict()
    for line in data.split("\n"):
        tokens = line.split(" ")
        match tokens[0]:
            case "---@param":
                out[tokens[1]] = line.replace(f"---@param {tokens[1]} ", "").replace("\n", "")
            case "---@return":
                out["return"] = line.replace("---@return ", "").replace("\n", "")
            case _:
                pass
    return out


def parse_lua_functions(data: str, module: str = "") -> Iterable[LuaFunc]:
    for match in re.finditer(PATTERN_LUA_FUNCTION, data, re.MULTILINE):
        comment = multiline_comment(match.group(1))
        fn_name = match.group(5)
        args: list[str] = str(match.group(6)).split(",")
        args = [x.strip() for x in args]
        ret = "any"
        if match.group(3):
            args = []
            parsed = parse_annotation(match.group(3))
            for k in parsed:
                if k == "return":
                    ret = parsed[k]
                else:
                    args.append(f"{k}: {parsed[k]}")
        (class_name, fn_name, with_self) = split_name(fn_name)
        if with_self:
            if len(args) == 1 and args[0] == "":
                args[0] = "self: self"
            else:
                args.insert(0, "self: self")
        if (class_name.lower(), fn_name.lower()) in docs:
            comment = multiline_comment(docs[(class_name.lower(), fn_name.lower())])
        if (module.lower(), fn_name.lower()) in docs:
            comment = multiline_comment(docs[(module.lower(), fn_name.lower())])
        yield LuaFunc(fn_name, class_name, ret, args, comment)


if __name__ == "__main__":
    docs = parse_docs()
    symbols_processing()
    signatures_processing()
    lua_modules_processing()
