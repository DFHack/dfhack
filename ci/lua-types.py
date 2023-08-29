import re
import xml.etree.ElementTree as ET
from collections.abc import Iterable
from dataclasses import dataclass
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

BASE_METHODS = """---@param id integer|number
---@return <BASE_TYPE>|nil
function <DECLARE_PREFIX>.find(id) end

---@param item any
---@return boolean
function <DECLARE_PREFIX>:is_instance(item) end

---@return <BASE_TYPE>
function <DECLARE_PREFIX>:new() end

---@param src <BASE_TYPE>
function <DECLARE_PREFIX>:assign(src) end

---@param item <BASE_TYPE>
---@return <BASE_TYPE>
function <DECLARE_PREFIX>:_displace(item) end

---@return boolean
function <DECLARE_PREFIX>:delete() end

---@return string
function <DECLARE_PREFIX>:__tostring() end

---@return integer
function <DECLARE_PREFIX>:sizeof() end
"""

df_global: list[str] = []
df: list[str] = []


class Tag:
    renderable = False
    el: ET.Element
    meta: str
    level: str
    name: str
    type_name: str
    typedef_name: str
    anon_name: str
    subtype: str
    ref_target: str
    pointer_type: str
    inherit: str
    comment: str
    since: str
    path: list[str]
    name_prefix: str = ""
    have_childs: bool = False
    invisible_field: bool = False

    def __init__(self, el: ET.Element, path: list[str] = []) -> None:
        self.el = el
        self.path = path
        self.parse()
        self.traverse()

    def parse(self) -> None:
        if len(self.el) == 1 and self.el[0].tag == "item":
            self.el.attrib = self.el[0].attrib | self.el.attrib
            self.renderable = False
        self.meta = self.el.attrib["meta"] if "meta" in self.el.attrib else ""
        self.level = self.el.attrib["level"] if "level" in self.el.attrib else ""
        self.name = self.el.attrib["name"] if "name" in self.el.attrib else ""
        self.type_name = self.el.attrib["type-name"] if "type-name" in self.el.attrib else ""
        self.typedef_name = self.el.attrib["typedef-name"] if "typedef-name" in self.el.attrib else ""
        self.anon_name = self.el.attrib["anon-name"] if "anon-name" in self.el.attrib else ""
        self.subtype = self.el.attrib["subtype"] if "subtype" in self.el.attrib else ""
        self.ref_target = self.el.attrib["ref-target"] if "ref-target" in self.el.attrib else ""
        self.pointer_type = self.el.attrib["pointer-type"] if "pointer-type" in self.el.attrib else ""
        self.inherit = self.el.attrib["inherits-from"] if "inherits-from" in self.el.attrib else ""
        self.since = self.el.attrib["since"] if "since" in self.el.attrib else ""
        self.comment = (
            " " + self.el.attrib["comment"] if "comment" in self.el.attrib and self.el.attrib["comment"] != "" else ""
        )
        if self.since:
            self.comment += " since " + self.since
        self.have_childs = len(self.el) > 0
        if not self.have_childs:
            self.renderable = False
        if len(self.path) > 0:
            self.name_prefix = ".".join(self.path) + "."

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

    def traverse(self) -> None:
        self.items = []
        shift = 0
        for i, child in enumerate(self.el, start=0):
            if child.tag != "enum-item":
                shift -= 1
                continue
            else:
                if "value" in child.attrib:
                    shift = int(child.attrib["value"]) - i
                self.items.append(EnumItem(child).set_index(i + shift))

    def render(self) -> str:
        name = f"{self.name_prefix}{self.typedef_name or self.type_name}"
        s = f"---@enum {name}{self.comment}\n"
        decl = "df."
        if len(self.path) > 0:
            s += f"---@diagnostic disable-next-line: undefined-global, inject-field\n"
            decl = "df.global." if self.path[0] in df_global else "df."
        s += f"{decl or 'df.'}{name} = {{\n"
        for item in self.items:
            s += f"{item.as_field()}\n"
        s += "}\n"
        return s

    def as_field(self) -> str:
        type_name = f"{self.name_prefix}{self.typedef_name or self.type_name}"
        field = f"---@field {self.name or self.anon_name} {type_name}{self.comment}"
        if self.typedef_name:
            field += f"\n---@field {self.typedef_name} {type_name} -- type for {self.name or self.anon_name}"
        return field

    def item_names(self) -> list[str]:
        names: list[str] = []
        shift = 0
        for i, child in enumerate(self.el, start=0):
            if child.tag != "enum-item":
                shift -= 1
                continue
            else:
                if "value" in child.attrib:
                    shift = int(child.attrib["value"]) - i
                item = EnumItem(child).set_index(i + shift)
                names.append(f'{item.name or ("unk_" + str(i)) }')
        return names


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
        inside_el = get_container_item(self.el)
        if self.have_childs and len(inside_el) > 0:
            inside_el.attrib = self.el.attrib | inside_el.attrib
            if "subtype" in inside_el.attrib and inside_el.attrib["subtype"] == "enum":
                item = Enum(inside_el, self.path)
            else:
                item = Compound(inside_el, self.path)
            self.renderable = True
            item.renderable = True
            self.items.append(item)

    def as_field(self) -> str:
        if len(self.items) > 0:
            type_name = f"{self.name_prefix}{self.items[0].typedef_name}"
        elif self.pointer_type and self.subtype == "stl-vector":
            type_name = self.pointer_type + "[]"
        else:
            type_name = ""
        subtype = "any[] -- NOT TYPED" if base_type(self.subtype) == "stl-vector" else base_type(self.subtype)
        field = f"---@field {self.name or self.anon_name} {base_type(self.type_name) or type_name or subtype or 'unknown -- NOT TYPED'}{self.comment}"
        if self.typedef_name:
            field += f"\n---@field {self.typedef_name} {base_type(self.type_name) or type_name or subtype or 'unknown'} -- type for {self.name or self.anon_name}"
        return field

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class Container(Tag):
    renderable = False
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []

        self.count = self.el.attrib["count"] if "count" in self.el.attrib else "0"
        inside_el = get_container_item(self.el)
        if self.have_childs and len(inside_el) > 0:
            inside_el.attrib = self.el.attrib | inside_el.attrib
            if "subtype" in inside_el.attrib and inside_el.attrib["subtype"] == "enum":
                item = Enum(inside_el, self.path)
            else:
                item = Compound(inside_el, self.path)
            self.renderable = True
            item.renderable = True
            self.items.append(item)

        self.inside_containter = (
            self.items[0].name_prefix + self.items[0].typedef_name
            if len(inside_el) > 0 and inside_el[0].tag != "code-helper"
            else ""
        )
        self.flagarray = (
            flagarray_type(self.el.attrib["index-enum"])
            if ("index-enum" in self.el.attrib and self.subtype == "df-flagarray")
            else ""
        )
        self.typed = f"{self.ref_target or base_type(self.type_name) or self.inside_containter or ('boolean' if self.subtype == 'stl-bit-vector' else '') or 'any'}[]"
        if self.flagarray and self.typed == "any[]":
            self.typed = self.flagarray

    def as_field(self) -> str:
        field = f"---@field {self.name or self.anon_name} {self.typed}{self.comment}{' -- NOT TYPED' if self.typed == 'any[]' else ''}"
        if len(self.items) > 0 and self.items[0].typedef_name:
            field += f"\n---@field {self.items[0].typedef_name} {self.typed} -- type for {self.name or self.anon_name}"
        return field

    def as_type(self) -> str:
        return f"---@type {self.typed}{self.comment}{' -- NOT TYPED' if self.typed == 'any[]' else ''}"

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class StaticArray(Tag):
    renderable = False
    count: str
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        self.count = self.el.attrib["count"] if "count" in self.el.attrib else "0"
        inside_el = get_container_item(self.el)
        if self.have_childs and len(inside_el) > 0:
            inside_el.attrib = self.el.attrib | inside_el.attrib
            if "subtype" in inside_el.attrib and inside_el.attrib["subtype"] == "enum":
                item = Enum(inside_el, self.path)
            else:
                item = Compound(inside_el, self.path)
            self.renderable = True
            item.renderable = True
            self.items.append(item)

    def as_field(self) -> str:
        inside_containter = self.items[0].name_prefix + self.items[0].typedef_name if len(self.items) > 0 else ""
        type_name = f"{self.ref_target or base_type(self.type_name) or inside_containter or 'any'}[]"
        return f"---@field {self.name or self.anon_name} {type_name} count<{self.count}>{self.comment}{' -- NOT TYPED' if type_name == 'any[]' else ''}"

    def as_type(self) -> str:
        inside_containter = self.items[0].name_prefix + self.items[0].typedef_name if len(self.items) > 0 else ""
        type_name = f"{self.ref_target or base_type(self.type_name) or inside_containter or 'any'}[]"
        return (
            f"---@type {type_name} count<{self.count}>{self.comment}{' -- NOT TYPED' if type_name == 'any[]' else ''}"
        )

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s


class Compound(Tag):
    renderable = True
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        for i, child in enumerate(self.el):
            for tag in switch_tag(child, self.path_to_child()):
                if not tag.name and not tag.anon_name:
                    tag.name = f"unnamed_{i}"
                    if not tag.type_name:
                        tag.type_name = tag.name
                self.items.append(tag)

    def as_field(self) -> str:
        type_name = f"{self.name_prefix}{self.typedef_name or self.type_name or 'anon-compound'}"
        field = f"---@field {self.name or self.anon_name} {type_name}{self.comment}"
        if self.typedef_name:
            field += f"\n---@field {self.typedef_name} {type_name} -- type for {self.name or self.anon_name}"
        return field

    def as_type(self) -> str:
        type_name = f"{self.name_prefix}{self.typedef_name or self.type_name}"
        return f"---@type {type_name}{self.comment}"

    def render(self) -> str:
        type_name = f"{self.name_prefix}{self.typedef_name or self.type_name or 'anon-compound'}"
        s = f"---@class {type_name}{self.comment}\n"
        append = ""
        for item in self.items:
            s += f"{item.as_field()}\n"
            if item.renderable:
                append += item.render()
        decl = declare(type_name)
        if decl != type_name:
            s += f"{decl} = nil\n"
        return s + "\n" + append

    def path_to_child(self) -> list[str]:
        path = self.path
        if self.typedef_name or self.type_name:
            path = path + [self.typedef_name or self.type_name]
        return path


class Struct(Tag):
    renderable = True
    items: list[Tag]

    def traverse(self) -> None:
        self.items = []
        for i, child in enumerate(self.el):
            for tag in switch_tag(child, self.path_to_child()):
                if not tag.name and not tag.anon_name:
                    tag.name = f"unnamed_{i}"
                self.items.append(tag)

    def render(self) -> str:
        type_name = f"{self.name_prefix}{self.type_name}"
        s = f"---@class {type_name}{': ' + self.inherit if self.inherit else ''}{self.comment}\n"
        append = "\n"
        for item in self.items:
            if not item.invisible_field:
                s += f"{item.as_field()}\n"
            if item.renderable:
                append += item.render()
        decl = declare(type_name)
        if decl != type_name:
            s += f"{decl} = nil\n"
            if not self.inherit:
                s += "\n" + base_methods(type_name, decl)
        return s + append

    def path_to_child(self) -> list[str]:
        path = self.path
        if self.type_name:
            path = path + [self.type_name]
        return path


class VirtualMethod(Tag):
    renderable = True
    ret: str = ""
    args: list[tuple[str, str]]

    def traverse(self) -> None:
        self.args = []
        if "ret-type" in self.el.attrib:
            self.ret = base_type(self.el.attrib["ret-type"])
        for i, child in enumerate(self.el):
            c = Tag(child)
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


class VirtualMethods(Tag):
    invisible_field = True
    renderable = True
    items: list[VirtualMethod]

    def traverse(self) -> None:
        self.items = []
        for child in self.el:
            self.items.append(VirtualMethod(child, self.path_to_child()))

    def render(self) -> str:
        s = ""
        for item in self.items:
            if item.renderable:
                s += item.render()
        return s

    def path_to_child(self) -> list[str]:
        path = self.path
        if self.type_name:
            path = path + [self.type_name]
        return path

    def as_field(self) -> str:
        return "-- VIRTUAL METHODS"


class DefaultGlobalField(Tag):
    renderable = False

    def as_field(self) -> str:
        return f"---@field {self.name or self.anon_name} {self.type_name}{self.comment}"


class GlobalObject(Tag):
    renderable = True
    items: list[Tag]

    def render(self) -> str:
        match self.meta:
            case "global" | "pointer":
                return f"---@type {self.type_name}{self.comment}\n{declare(self.name)} = nil\n"
            case "number":
                return f"---@type {base_type(self.subtype)}{self.comment}\n{declare(self.name)} = nil\n"
            case "static-array":
                tag = StaticArray(self.el[0])
                return f"{tag.as_type()}\n{declare(self.name)} = nil\n"
            case "compound":
                tag = Compound(self.el[0])
                return f"{tag.as_type()}\n{declare(self.name)} = nil\n\n{tag.render()}"
            case "container":
                tag = Container(self.el[0])
                return f"{tag.as_type()}\n{declare(self.name)} = nil\n"
            case _:
                return "-- NOT HANDLED GLOBAL OBJECT " + self.name


def parse_codegen(file: Path):
    tree = ET.parse(file)
    root = tree.getroot()
    with Path(PATH_DEFINITIONS).open("w", encoding="utf-8") as output:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=output)
        for el in root:
            for tag in switch_global_tag(el):
                print(tag.render(), file=output)


def base_methods(base_type: str, declare_prefix: str) -> str:
    return BASE_METHODS.replace("<BASE_TYPE>", base_type).replace("<DECLARE_PREFIX>", declare_prefix)


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


def get_container_item(el: ET.Element, force: bool = True) -> ET.Element:
    while len(el) > 0 and ("is-container" in el.attrib) and el.attrib["is-container"] == "true":
        el = el[0]
    return el


def flagarray_type(index_enum: str) -> str:
    if index_enum in enum_map:
        items = enum_map[index_enum].item_names()
        if len(items) > 0:
            s = f"table<"
            for item in items:
                s += f'"{item}"|'
            s = f"{s[:-1]}, boolean>"
            return s
    return "table<string, boolean>"


def declare(name: str) -> str:
    if name in df_global:
        return "df.global." + name
    if name in df:
        return "df." + name
    return name


def switch_tag(el: ET.Element, path: list[str] = []) -> Iterable[Tag]:
    if el.tag != "item" and el.tag != "code-helper":
        tag = Tag(el)
        match tag.meta:
            case "primitive":
                yield Primitive(el, path)
            case "bytes":
                yield Primitive(el, path)
            case "number":
                yield Primitive(el, path)
            case "pointer":
                yield Pointer(el, path)
            case "container":
                yield Container(el, path)
            case "static-array":
                yield StaticArray(el, path)
            case "compound":
                match tag.subtype:
                    case "enum":
                        tag = Enum(el, path)
                        enum_map[tag.type_name] = tag
                        yield tag
                    case _:
                        yield Compound(el, path)
            case "global":
                match tag.subtype:
                    case "enum":
                        yield Enum(el)
                    case _:
                        yield DefaultGlobalField(el)
                pass
            case _:
                if el.tag == "virtual-methods":
                    yield VirtualMethods(el, path)
                else:
                    print("Skip tag ->", el.tag)


def switch_global_tag(el: ET.Element) -> Iterable[Tag]:
    tag = Tag(el)
    match tag.meta:
        case "enum-type":
            tag = Enum(el)
            enum_map[tag.type_name] = tag
            yield tag
        case "struct-type":
            yield Struct(el)
        case "class-type":
            yield Struct(el)
        case "bitfield-type":
            yield Struct(el)
        case _:
            match el.tag:
                case "global-object":
                    yield GlobalObject(el)
                case _:
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
