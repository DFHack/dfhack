import re
import xml.etree.ElementTree as ET
from collections.abc import Iterable
from dataclasses import dataclass
from enum import Enum as EnumType
from pathlib import Path
from typing import Any

########################################
#        Signatures processing         #
########################################


PATH_LUAAPI = "./library/LuaApi.cpp"
PATH_LUATOOLS = "./library/LuaTools.cpp"
PATH_LIBRARY = "./library/"
PATH_DFHACK_OUTPUT = "./types/library/dfhack.lua"
PATH_DEFAULT = "./types/library/default.lua"

PATTERN_WRAPM = r"WRAPM\((.+), (.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_CWRAP = r"CWRAP\((.+), (.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_WRAPN = r"WRAPN\((.+), (.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_WRAP = r"WRAP\((.+)\)[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_MODULE_ARRAY = r"dfhack_[\w_]*module\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ARRAY = r"dfhack_[\w_]*funcs\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ITEM = r"\{\s\"(\w+)\",\s(\w+)\s\}[,](\s?\/\/\s?<expose>\s?.*)*"
PATTERN_SIGNATURE = r"^([\w:<>]+)(.+)?\(([^)]*)\)"
PATTERN_SIGNATURE_SEARCH = r"^.+[\s\*]+(DFHack::){0,1}module::function[\s]{0,1}\([\n]{0,1}(.|,\n)*?\n{0,1}\)[\s\n\w]+\{"
PATTERN_FORCE_EXPOSE = r"\/\/\s?<force-expose>\s?(.+)"


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
        with Path(path).open("r", encoding="utf-8") as file:
            data = file.read()
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
                    decoded_signature = decode_signature(match.group(1).strip())
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
    module = module_name(array)
    if not module:
        module = ""
    for match in re.finditer(PATTERN_WRAP, array):
        item = Entry(module, match.group(1).split(",")[0], "WRAP", None, None)
        if match.group(2):
            print("EXPOSE")
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
        with entry.open("r", encoding="utf-8") as file:
            data = file.read()
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


def decode_signature(sig: str) -> Signature | None:
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
                    decoded_type_arg = decode_type(arg_type)
                    if decoded_type_arg in ["lua_State", "color_ostream", "MapExtras::MapCache"]:
                        continue
                arg = Arg(
                    name=arg_name,
                    type=decoded_type_arg.replace("::", "__"),
                    default_value=default_value,
                    unknown=(decoded_type_arg == arg_type and arg_type != "string")
                    or any(c.isupper() for c in decoded_type_arg),
                    vararg=vararg,
                )
                args.append(arg)
        if decoded_type_ret:
            return Signature(
                ret=Ret(
                    type=decoded_type_ret.replace("::", "__").replace("enums__biome_type__", ""),
                    unknown=decoded_type_ret == type_ret and type_ret != "string",
                ),
                args=check_optional_bool(args),
                name=match.group(2).strip().lower().replace("::", ".") if match.group(2) else None,
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
    s = f"-- CXX SIGNATURE -> {entry.sig}\n"
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
        out += f"\n---@alias env {{ df: df, dfhack: dfhack }}\n\n---@param module string\n---@return env\nfunction mkmodule(module) end\n\n"
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
                    unknown_types.add(f"---@alias {entry.decoded_sig.ret.type.replace('[]', '')} unknown\n")
                for arg in entry.decoded_sig.args:
                    if arg.unknown:
                        unknown_types.add(f"---@alias {arg.type.replace('[]', '')} unknown\n")
            print(print_entry(entry), file=dest)
        if unknown_types.__len__() > 0:
            print(f"\n-- Unknown types\n{''.join(unknown_types)}", file=dest)

    generate_base_methods()


########################################
#          Symbols processing          #
########################################

PATH_CODEGEN = "./library/include/df/codegen.out.xml"
PATH_DEFINITIONS = "./types/library/definitions.lua"

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
    render_mode: RenderMode
    have_childs: bool = False
    is_container: bool = False

    def __init__(self, el: ET.Element, render_mode: RenderMode, path: list[str] = []) -> None:
        self.el = el
        self.path = path
        self.render_mode = render_mode
        self.parse()
        self.traverse()

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

    def traverse(self) -> None:
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

    def traverse(self) -> None:
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
        s += f"---@diagnostic disable-next-line: undefined-global, inject-field\n"
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


class Primitive(Tag):
    renderable = False

    def as_field(self) -> str:
        return f"---@field {self.name or self.anon_name} {base_type(self.subtype)}{self.comment}"


class Pointer(Tag):
    renderable = False
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        (inside_el, self.array_dim) = get_container_item(self.el)
        if self.render_mode == RenderMode.Type and not self.typedef_name and "typedef-name" in inside_el.attrib:
            self.typedef_name = inside_el.attrib["typedef-name"]
            self.path = self.path[:-1]
            self.path.append(self.typedef_name)
            self.full_type = ".".join(self.path)

        if self.have_childs and len(inside_el) > 0:
            inside_el.attrib = self.el.attrib | inside_el.attrib
            if inside_el.attrib.get("subtype") == "enum":
                item = Enum(inside_el, self.render_mode, self.path[:-1])
            else:
                item = Compound(inside_el, self.render_mode, self.path[:-1])
            self.renderable = True
            item.renderable = True
            self.items.append(item)

        if len(self.items) > 0:
            type_name = f"{self.full_type}"
        elif self.pointer_type and self.subtype == "stl-vector":
            type_name = self.pointer_type + "[]"
        else:
            type_name = ""
        subtype = "any[] -- NOT TYPED" if base_type(self.subtype) == "stl-vector" else base_type(self.subtype)

        self.typed = base_type(self.type_name) or type_name or subtype or self.full_type

        if inside_el.attrib.get("meta") == "global":
            self.typed = inside_el.attrib["type-name"] + ("[]" * len(self.array_dim))
        if inside_el.attrib.get("meta") == "primitive" or inside_el.attrib.get("meta") == "number":
            self.typed = base_type(inside_el.attrib["subtype"]) + ("[]" * len(self.array_dim))
        if (
            len(self.array_dim) > 0
            and inside_el.attrib.get("meta") == "container"
            and inside_el.attrib["subtype"] == "stl-vector"
        ):
            self.typed = "unknown" + ("[]" * len(self.array_dim))

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


class Container(Tag):
    renderable = True
    is_container = True
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        self.renderable = True
        (inside_el, self.array_dim) = get_container_item(self.el)
        if self.render_mode == RenderMode.Type and not self.typedef_name and inside_el.attrib.get("typedef-name"):
            self.typedef_name = inside_el.attrib["typedef-name"]
            self.path = self.path[:-1]
            self.path.append(self.typedef_name)
            self.full_type = ".".join(self.path)

        if self.have_childs and len(inside_el) > 0:
            inside_el.attrib = self.el.attrib | inside_el.attrib
            if inside_el.attrib.get("subtype") == "enum":
                item = Enum(inside_el, self.render_mode, self.path[:-1])
            else:
                item = Compound(inside_el, self.render_mode, self.path[:-1])
            item.renderable = True
            self.items.append(item)

        inside_containter = self.full_type if len(inside_el) > 0 and inside_el[0].tag != "code-helper" else ""
        self.typed = f"{self.ref_target or base_type(self.type_name) or inside_containter or ('boolean' if self.subtype == 'stl-bit-vector' else '') or 'any'}{'[]' * len(self.array_dim)}"

        if self.subtype == "df-flagarray":
            if "index-enum" in self.el.attrib and self.el.attrib["index-enum"] in enum_map:
                self.typed = "table<"
                for key in enum_map[self.el.attrib["index-enum"]].keys():
                    self.typed += f'"{key}"|'
                self.typed = self.typed[:-1] + ", boolean>"
            else:
                self.typed = "table<string, boolean>"

        if inside_el.attrib.get("meta") == "global":
            self.typed = inside_el.attrib["type-name"] + ("[]" * len(self.array_dim))
        if inside_el.attrib.get("meta") == "primitive" or inside_el.attrib.get("meta") == "number":
            self.typed = base_type(inside_el.attrib["subtype"]) + ("[]" * len(self.array_dim))
        if (
            len(self.array_dim) > 0
            and inside_el.attrib.get("meta") == "container"
            and inside_el.attrib["subtype"] == "stl-vector"
        ):
            self.typed = "unknown" + ("[]" * len(self.array_dim))
        if inside_el.attrib.get("subtype") == "stl-string" and self.pointer_type == "stl-string":
            self.typed = f"{{ value: string }}{'[]' * len(self.array_dim)}"

        if self.typed.startswith(self.full_type + "[]"):
            if self.render_mode == RenderMode.Type:
                self.renderable = True
            if not self.have_childs and not self.type_name:
                self.typed = "unknown -- NOT TYPED"

    def as_field(self) -> str:
        count = f" count<{','.join(self.array_dim)}>"
        type_name = self.full_type
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
            s += f"---@class {self.full_type}{(': ' + self.inherit) if self.inherit else ''}{self.comment}\n"
            s += f"---@field [integer] {self.typed[:-2] if self.typed.endswith('[]') else self.typed}\n"
            s += BASE_CONTAINER_METHODS.replace(
                "<UNDERLAYING>", self.typed[:-2] if self.typed.endswith("[]") else self.typed
            )
            if self.render_mode == RenderMode.Type:
                s += BASE_NAMED_TYPES_METHODS
            else:
                s += BASE_TYPED_OBJECTS_METHODS
            s += f"df.{self.full_type} = nil\n\n"
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class StaticArray(Tag):
    renderable = False
    is_container = True
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        (inside_el, self.array_dim) = get_container_item(self.el)
        if self.render_mode == RenderMode.Type and not self.typedef_name and "typedef-name" in inside_el.attrib:
            self.typedef_name = inside_el.attrib["typedef-name"]
            self.path = self.path[:-1]
            self.path.append(self.typedef_name)
            self.full_type = ".".join(self.path)

        if self.have_childs and len(inside_el) > 0:
            if inside_el.attrib.get("name") and self.el.attrib.get("name"):
                inside_el.attrib["name"] = self.el.attrib["name"]
            inside_el.attrib = self.el.attrib | inside_el.attrib
            if inside_el.attrib.get("subtype") == "enum":
                item = Enum(inside_el, self.render_mode, self.path[:-1])
            else:
                item = Compound(inside_el, self.render_mode, self.path[:-1])
            self.renderable = True
            item.renderable = True
            self.items.append(item)

        inside_containter = self.full_type if len(inside_el) > 0 and inside_el[0].tag != "code-helper" else ""
        self.typed = (
            f"{self.ref_target or base_type(self.type_name) or inside_containter or 'any'}{'[]' * len(self.array_dim)}"
        )

        if (
            inside_el.attrib.get("meta") == "global"
            or inside_el.attrib.get("meta") == "number"
            or inside_el.attrib.get("meta") == "primitive"
        ):
            item_type = inside_el.attrib.get("type-name") or inside_el.attrib.get("subtype") or "any"
            if "index-enum" in self.el.attrib and self.el.attrib["index-enum"] in enum_map:
                self.typed = "table<"
                for key in enum_map[self.el.attrib["index-enum"]].keys():
                    self.typed += f'"{key}"|'
                self.typed = self.typed[:-1] + f", {base_type(item_type)}>"
            else:
                self.typed = base_type(item_type) + ("[]" * len(self.array_dim))
        if (
            len(self.array_dim) > 0
            and inside_el.attrib.get("meta") == "container"
            and inside_el.attrib["subtype"] == "stl-vector"
        ):
            self.typed = "unknown" + ("[]" * len(self.array_dim))

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

    def traverse(self) -> None:
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
                    self.items.append(tag)

    def as_field(self) -> str:
        return f"---@field {self.path[-1]} {self.full_type}{self.comment}"

    def as_type(self) -> str:
        return f"---@type {self.full_type}{self.comment}"

    def render(self) -> str:
        s = f"-- {self.render_mode}, path: {self.path}, compound\n"
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

    def traverse(self) -> None:
        self.items = []
        for mode in [RenderMode.Regular, RenderMode.Type]:
            for i, child in enumerate(self.el):
                for tag in switch_tag(child, mode, self.path[:]):
                    if not tag.name and not tag.anon_name:
                        tag.name = f"unnamed_{i}"
                    if (mode == RenderMode.Type and tag.typedef_name) or (mode == RenderMode.Regular and tag.name):
                        if tag.is_container:
                            self.has_container = True
                        self.items.append(tag)

    def render(self) -> str:
        s = f"-- {self.render_mode}, path: {self.path}, struct\n"
        s += f"---@class {self.full_type}{(': ' + self.inherit) if self.inherit else ''}{self.comment}\n"
        append = ""
        for item in self.items:
            if item.fieldable:
                s += f"{item.as_field()}\n"
            if item.renderable:
                append += item.render()
        if self.instance_vector:
            s += "---@field find fun(id: integer): self|nil\n"
        if not self.inherit:
            s += BASE_METHODS + BASE_STRUCT_METHODS + BASE_TYPED_OBJECTS_METHODS
        s += f"df.{self.full_type} = nil\n"
        return s + "\n" + append


# TODO: unused, delete later
class Union(Tag):
    renderable = True
    fieldable = True
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        for i, child in enumerate(self.el):
            for tag in switch_tag(child, self.render_mode, self.path[:-1]):
                if not tag.name and not tag.anon_name:
                    tag.name = f"unnamed_{i}"
                if (self.render_mode == RenderMode.Type and tag.typedef_name) or (
                    self.render_mode == RenderMode.Regular and tag.name
                ):
                    self.items.append(tag)

    def as_field(self) -> str:
        s = "-- union start\n"
        for item in self.items:
            if item.fieldable:
                s += f"{item.as_field()}\n"
        return s + "-- union end"

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s + "\n"


# TODO: unused, delete later
class VirtualMethod(Tag):
    renderable = True
    ret: str = ""
    args: list[tuple[str, str]]

    def traverse(self) -> None:
        self.args = []
        if "ret-type" in self.el.attrib:
            self.ret = base_type(self.el.attrib["ret-type"])
        for i, child in enumerate(self.el):
            c = Tag(child, self.render_mode)
            type_name = ""
            if c.subtype == "stl-vector":
                type_name = f"{c.ref_target or base_type(c.pointer_type) or base_type(c.type_name) or 'any'}[]"
            match child.tag:
                case "ret-type":
                    self.ret = (
                        type_name
                        or base_type(c.ref_target)
                        or base_type(c.type_name)
                        or base_type(c.pointer_type)
                        or base_type(c.subtype)
                        or c.el.tag
                    )
                    continue
                case "comment":
                    continue
                case _:
                    arg_name = c.name or c.anon_name or f"arg_{i}"
                    if arg_name == "local":
                        arg_name = f"arg_{i}"
                    arg_type = (
                        type_name
                        or base_type(c.type_name)
                        or base_type(c.pointer_type)
                        or base_type(c.subtype)
                        or "unknown"
                    )
                    self.args.append((arg_name, arg_type))

    def render(self) -> str:
        params: list[str] = []
        s = ""
        for a in self.args:
            s += f"---@param {a[0]} {a[1]}\n"
            params.append(a[0])
        if self.ret != "":
            s += f"---@return {base_type(self.ret)}\n"
        decl = ""
        if len(self.path) > 0:
            if self.path[0] in df_global:
                decl += "df.global."
            elif self.path[0] in df:
                decl += "df."
            decl += ".".join(self.path)
            decl += "."
        s += f"function {decl}{self.name or self.anon_name}({', '.join(params)}) end{' --' + self.comment if self.comment != '' else ''}\n\n"
        return s

    def as_field(self) -> str:
        signature = "fun("
        for a in self.args:
            signature += f"{a[0]}: {a[1]},"
        if signature[-1] == ",":
            signature = signature[:-1]
        signature = f"{signature}): {self.ret or 'nil'}"
        name = self.name
        if self.name.split(".").__len__() > 1:
            name = self.name.split(".")[-1]
        return f"---@field {name} {signature}{self.comment}"


# TODO: unused, delete later
class VirtualMethods(Tag):
    renderable = True
    fieldable = False
    items: list[VirtualMethod]

    def traverse(self) -> None:
        self.fieldable = False
        self.items = []
        for child in self.el:
            self.items.append(VirtualMethod(child, self.render_mode, self.path[:-1]))

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s

    def as_field(self) -> str:
        return "-- VIRTUAL METHODS"


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


def parse_codegen(file: Path):
    tree = ET.parse(file)
    root = tree.getroot()
    total = 0
    with Path(PATH_DEFINITIONS).open("w", encoding="utf-8") as output:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=output)
        for el in root:
            for tag in switch_global_tag(el):
                total += 1
                print(tag.render(), file=output)
    print(f"Tags: {total}")


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


# TODO: unused, delete later
def declare(name: str) -> str:
    if name in df_global:
        return "df.global." + name
    return "df." + name


def switch_tag(el: ET.Element, render_mode: RenderMode, path: list[str] = []) -> Iterable[Tag]:
    if el.tag != "item" and el.tag != "code-helper":
        tag = Tag(el, render_mode)
        match tag.meta:
            case "primitive" | "bytes" | "number":
                yield Primitive(el, render_mode, path)
            case "pointer":
                yield Pointer(el, render_mode, path)
            case "container":
                yield Container(el, render_mode, path)
            case "static-array":
                yield StaticArray(el, render_mode, path)
            case "compound":
                match tag.subtype:
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
                match tag.subtype:
                    case "enum":
                        yield Enum(el, render_mode, path)
                    case _:
                        yield DefaultGlobalField(el, render_mode, path)
                pass
            case _:
                # if el.tag == "virtual-methods":
                #     yield VirtualMethods(el, path, render_mode)
                # else:
                print("Skip tag ->", el.tag)


def switch_global_tag(el: ET.Element) -> Iterable[Tag]:
    tag = Tag(el, RenderMode.Regular)
    match tag.meta:
        case "enum-type":
            yield Enum(el, RenderMode.Regular, [])
        case "struct-type" | "class-type" | "bitfield-type":
            yield Struct(el, RenderMode.All, [])
        case _:
            match el.tag:
                case "global-object":
                    yield GlobalObject(el, RenderMode.Regular, [])
                case _:
                    pass
                    print(f"-- SKIPPED TAG {el.tag} {tag.meta} {tag.type_name}")


def parse_items_place() -> None:
    for file in sorted(Path(PATH_XML).glob("*.xml")):
        with file.open("r", encoding="utf-8") as src:
            data = src.read()
            for match in re.finditer(r"(struct|class|enum)-type.*type-name='([^']+).*'", data, re.MULTILINE):
                df.append(match.group(2))
            for match in re.finditer(r"(global-object).*\sname='([^']+).*'", data, re.MULTILINE):
                df_global.append(match.group(2))


def symbols_processing() -> None:
    if not Path(PATH_OUTPUT).is_dir():
        Path(PATH_OUTPUT).mkdir(parents=True, exist_ok=True)
        with Path(PATH_LIB_CONFIG).open("w", encoding="utf-8") as dest:
            print(LIB_CONFIG, file=dest)
        with Path(PATH_CONFIG).open("w", encoding="utf-8") as dest:
            print(CONFIG, file=dest)

    data = ""
    tmp = Path(PATH_CODEGEN + ".tmp")
    with Path(PATH_CODEGEN).open("r", encoding="utf-8") as src:
        data = src.read()
        data = data.replace("ld:", "")
    with tmp.open("w", encoding="utf-8") as dest:
        dest.write(data)
    parse_items_place()
    parse_codegen(tmp)
    tmp.unlink()


if __name__ == "__main__":
    symbols_processing()
    signatures_processing()
