import re
import xml.etree.ElementTree as ET
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Any
from xmlrpc.client import boolean

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
  "workspace.ignoreDir": ["build"]
}}\n"""

BASE_METHODS = """
---@param id integer|number
---@return <BASE_TYPE>|nil
function <DECLARE_PREFIX>.find(id) end

---@param item any
---@return boolean
function <DECLARE_PREFIX>:is_instance(item) end
"""

########################################
#          Symbols processing          #
########################################

df_global: list[str] = []
df: list[str] = []
class_map: dict[str, str] = {}


class Tag:
    el: ET.Element
    name: str
    type: str
    comment: str
    tag: str
    naming_rule: list[str]
    default_name: str
    name_prefix: str
    type_prefix: str
    declare_prefix: str
    have_children: boolean = False

    def __init__(
        self,
        el: ET.Element,
        naming_rule: list[str] = ["name", "type-name"],
        default_name: str = "unknown",
        name_prefix: str = "",
        type_prefix: str = "",
        declare_prefix: str = "",
    ) -> None:
        self.el = el
        self.tag = el.tag
        self.naming_rule = naming_rule
        self.default_name = default_name
        self.name_prefix = name_prefix
        self.type_prefix = type_prefix
        self.declare_prefix = declare_prefix
        self.have_children = self.el.__len__() > 0
        self.parse()

    def parse(self) -> None:
        self.name = fetch_name(self.el, self.naming_rule, self.default_name)
        if self.name_prefix != "":
            self.name = f"{self.name_prefix}{self.name}"
        self.type = fetch_type(self.el, self.type_prefix)
        self.comment = fetch_comment(self.el)

    def render(self) -> str:
        return "abstract render"


class EnumItem(Tag):
    index: int = 0

    def render(self) -> str:
        if self.comment:
            self.comment = " --" + self.comment
        return f"{self.name} = {self.index},{self.comment}"

    def set_index(self, index: int):
        self.index = index
        return self


class Enum(Tag):
    def render(self) -> str:
        s = f"---@enum {self.name}{self.comment}\n"
        s += f"{declare_prefix(self.name, 'df.')}{self.name} = {{\n"
        shift = 0
        for i, child in enumerate(self.el, start=0):
            if child.tag != "enum-item":
                shift -= 1
                continue
            else:
                if "value" in child.attrib:
                    shift = int(child.attrib["value"]) - i
                s += line(
                    EnumItem(child, naming_rule=["name"], default_name=f"unk_{i}").set_index(i + shift).render(), 4
                )
        s += "}\n"
        return s

    def as_field(self) -> str:
        t = self.type
        if self.have_children:
            t = self.name_prefix + self.el.attrib["name"] if "name" in self.el.attrib else self.default_name
        else:
            t = (
                self.el.attrib["type-name"]
                if "type-name" in self.el.attrib
                else self.el.attrib["name"]
                if "name" in self.el.attrib
                else self.default_name
            )
        return field(self.name.split(".")[-1], t, self.comment)

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
                item = EnumItem(child, naming_rule=["name"], default_name=f"unk_{i}").set_index(i + shift)
                names.append(f'"{item.name}"')
        return names


class Bitfield(Tag):
    def render(self) -> str:
        if self.have_children:
            s = f"---@alias {self.name} {self.get_type()}{self.comment}\n"
            s += f"---@type {self.name}\n"
            s += f"{declare_prefix(self.name, 'df.')}{self.name} = nil\n"
            return s
        else:
            return f"---@alias {self.name} unknown{self.comment}\n\n"

    def get_type(self) -> str:
        items: list[Tag] = []
        for i, child in enumerate(self.el):
            if child.tag != "flag-bit":
                continue
            items.append(Tag(child, default_name=f"unk_{i}"))
        if items.__len__() > 0:
            s = f"table<"
            for item in items:
                s += f'"{item.name}"|'
            s = f"{s[:-1]}, boolean>"
            return s
        else:
            return "table<string, boolean>"

    def as_field(self) -> str:
        n = self.name
        if self.name.split(".").__len__() > 1:
            n = self.name.split(".")[1]
        if self.have_children:
            return field(n, self.get_type(), self.comment)
        else:
            return field(n, self.name, self.comment)


enum_map: dict[str, Enum] = {}


class Struct(Tag):
    def render(self) -> str:
        declare = declare_prefix(self.name)
        inheritance = ": " + self.el.attrib["inherits-from"] if "inherits-from" in self.el.attrib else ""
        s = f"---@class {self.name}{inheritance}{self.comment}\n"
        childs: list[Tag] = []

        for index, child in enumerate(self.el, start=1):
            c = Tag(child, naming_rule=["name"], default_name=f"unnamed_{self.name}_{index}", type_prefix=self.name)
            if c.tag == "custom-methods" or c.type == "code-helper" or c.type == "comment":
                continue
            if c.tag == "enum":
                name_prefix = f"T_" if c.have_children else ""
                enum = Enum(
                    el=child,
                    name_prefix=f"{self.name_prefix}{self.name}.{name_prefix}",
                    default_name=f"{self.name}_unknown_enum_{index}",
                    declare_prefix=f"{declare}{self.name_prefix}{self.name}_",
                )
                enum_map[enum.name] = enum
                if enum.have_children:
                    childs.append(enum)
                s += enum.as_field()
                continue
            if c.tag == "compound" and c.have_children:
                if "is-union" not in c.el.attrib:
                    childs.append(Struct(c.el, name_prefix=f"{self.name}_"))
            if c.tag == "virtual-methods":
                childs.append(VirtualMethods(c.el, name_prefix=f"{declare}{self.name}."))
                continue
            if c.tag == "df-flagarray":
                flagarray = DfFlagArray(
                    el=child,
                    naming_rule=["name"],
                    default_name=f"unnamed_{self.name}_{index}",
                    name_prefix=f"{self.name_prefix}{self.name}.",
                )
                childs.append(flagarray)
                s += flagarray.as_field()
                continue
            if c.tag == "bitfield":
                bitfield = Bitfield(
                    el=child,
                    default_name=f"unnamed_{self.name}_{index}",
                    name_prefix=f"{self.name_prefix}{self.name}.",
                )
                if not bitfield.have_children:
                    childs.append(bitfield)
                s += bitfield.as_field()
                continue
            s += field(c.name, c.type, c.comment)

        class_map[self.name] = s
        if declare != "":
            s += f"{declare}{self.name_prefix}{self.name} = nil\n"
        else:
            s += "\n"
        for child in childs:
            s = child.render() + s
        if not inheritance and declare:
            s = base_methods(self.name, f"{declare}{self.name_prefix}{self.name}") + "\n" + s

        return s


class DfOtherVector(Tag):
    def render(self) -> str:
        s = f"---@class {self.name}{self.comment}\n"
        for index, child in enumerate(self.el, start=0):
            name = fetch_name(child, ["name"], f"unnamed_{index}")
            if child.tag != "stl-vector":
                s += "UNKNOWN VECTOR FIELD"
            s += field(name, fetch_type(child, self.name), fetch_comment(child))

        return s


class VirtualMethod(Tag):
    def render(self) -> str:
        ret = self.get_ret()
        args = self.get_args()
        s = ""
        params: list[str] = []
        for a in args:
            s += f"---@param {a[0]} {a[1]}\n"
            params.append(a[0])
        if ret != "":
            s += f"---@return {base_type(ret)}\n"
        s += f"function {self.name}({', '.join(params)}) end{' --' + self.comment if self.comment != '' else ''}\n\n"
        return s

    def as_field(self) -> str:
        ret = self.get_ret() or "nil"
        args = self.get_args()
        signature = "fun("
        for a in args:
            signature += f"{a[0]}: {a[1]},"
        if signature[-1] == ",":
            signature = signature[:-1]
        signature = f"{signature}): {ret}"
        name = self.name
        if self.name.split(".").__len__() > 1:
            name = self.name.split(".")[-1]
        return field(name, signature, self.comment)

    def get_ret(self) -> str:
        ret = self.el.attrib["ret-type"] if "ret-type" in self.el.attrib else ""
        for child in self.el:
            if child.tag == "ret-type":
                ret = fetch_type(child[0], "")
                break
        return base_type(ret)

    def get_args(self) -> list[tuple[str, str]]:
        args: list[tuple[str, str]] = []
        for i, child in enumerate(self.el):
            if child.tag == "comment" or child.tag == "ret-type":
                continue
            args.append(
                (
                    child.attrib["name"] if "name" in child.attrib and child.attrib["name"] != "local" else f"arg_{i}",
                    fetch_type(child, ""),
                )
            )
        return args


class VirtualMethods(Tag):
    def render(self) -> str:
        s = ""
        for i, child in enumerate(self.el):
            if "is-destructor" in child.attrib:
                continue
            s += VirtualMethod(
                child, name_prefix=f"{self.name_prefix}", naming_rule=["name"], default_name=f"unnamed_method_{i}"
            ).render()
        return s

    def get_methods(self) -> list[VirtualMethod]:
        methods: list[VirtualMethod] = []
        for i, child in enumerate(self.el):
            if "is-destructor" in child.attrib:
                continue
            methods.append(
                VirtualMethod(
                    child, name_prefix=f"{self.name_prefix}", naming_rule=["name"], default_name=f"unnamed_method_{i}"
                )
            )
        return methods


class GlobalObject(Tag):
    def render(self) -> str:
        if self.have_children:
            self.type = fetch_type(self.el[0])
        return f"---@type {self.type}{self.comment}\ndf.global.{self.name} = nil\n"


class DfLinkedList(Tag):
    def render(self) -> str:
        return f"---@alias {self.name} {self.el.attrib['item-type']}[]{self.comment}\n"


class DfFlagArray(Tag):
    def render(self) -> str:
        if "index-enum" in self.el.attrib:
            return ""
        else:
            return f"---@alias {self.name} unknown{self.comment}\n\n"

    def as_field(self) -> str:
        if "index-enum" in self.el.attrib:
            if self.el.attrib["index-enum"] in enum_map:
                enum = enum_map[self.el.attrib["index-enum"]]
                enum_names = enum.item_names()
                if enum_names.__len__() > 0:
                    return field(self.name.split(".")[1], f"table<{'|'.join(enum_names)}, boolean>", self.comment)
            return field(self.name, f"table<string, boolean>", self.comment)
        else:
            return field(self.name.split(".")[1], self.name, self.comment)


def parse_xml(file: Path) -> str:
    tree = ET.parse(file)
    root = tree.getroot()
    result: str = "-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n"
    for child in root:
        for tag in parse_tag(child):
            result += tag.render() + "\n"
    return result


def parse_tag(el: ET.Element) -> Iterable[Tag]:
    match el.tag:
        case "enum-type":
            e = Enum(el)
            enum_map[e.name] = e
            yield e
        case "struct-type":
            yield Struct(el)
        case "class-type":
            yield Struct(el)
        case "global-object":
            yield GlobalObject(el)
        case "bitfield-type":
            yield Bitfield(el)
        case "df-other-vectors-type":
            yield DfOtherVector(el)
        case "df-linked-list-type":
            yield DfLinkedList(el)
        case _:
            print(f"-- SKIPPED TAG {el.tag}")


def base_methods(base_type: str, declare_prefix: str) -> str:
    return BASE_METHODS.replace("<BASE_TYPE>", base_type).replace("<DECLARE_PREFIX>", declare_prefix)


def line(text: str, intend: int = 0) -> str:
    prefix = "".join(" " * intend)
    return f"{prefix}{text}\n"


def field(name: str, type: str, comment: str = "", intend: int = 0) -> str:
    return line("---@field " + name + " " + type + comment)


def base_type(type: str) -> str:
    match type:
        case "int8_t" | "uint8_t" | "int16_t" | "uint16_t" | "int32_t" | "uint32_t" | "int64_t" | "uint64_t" | "size_t":
            return "integer"
        case "stl-string" | "static-string" | "ptr-string":
            return "string"
        case "s-float" | "d-float" | "long" | "ulong":
            return "number"
        case "bool":
            return "boolean"
        case "pointer" | "padding":
            return "integer"  # or what?
        case "stl-bit-vector":
            return "boolean[]"
        case _:
            return type


def fetch_name(el: ET.Element, options: list[str], default: str = "WTF_UNKNOWN_NAME") -> str:
    for item in options:
        if item in el.attrib:
            return el.attrib[item]
    return default


def fetch_type(el: ET.Element, prefix: str = "") -> str:
    type = (
        el.attrib["type-name"]
        if "type-name" in el.attrib
        else el.attrib["pointer-type"]
        if "pointer-type" in el.attrib
        else el.tag
    )

    match el.tag:
        case "df-flagarray":
            return (el.attrib["index-enum"] if "index-enum" in el.attrib else "any") + "[]"
        case "pointer":
            return "any[]" if "is-array" in el.attrib else base_type(type)
        case "stl-vector" | "static-array":
            if "type-name" in el.attrib:
                return base_type(el.attrib["type-name"]) + "[]"
            elif "pointer-type" in el.attrib:
                return base_type(el.attrib["pointer-type"]) + "[]"
            else:
                return "any[]"
        case "bitfield":
            return "bitfield"
        case "compound":
            if "type-name" in el.attrib:
                return el.attrib["type-name"]
            if "pointer-type" in el.attrib:
                return el.attrib["pointer-type"]
            if "is-union" in el.attrib and el.attrib["is-union"] == "true":
                t: list[str] = []
                for child in el:
                    t.append(base_type(child.attrib["name"] if "name" in child.attrib else child.tag))
                return " | ".join(t) if t.__len__() > 0 else base_type(type)
            else:
                return prefix + "_" + fetch_name(el, ["name", "type-name"], "unknown")
        case _:
            return base_type(type)


def fetch_comment(el: ET.Element) -> str:
    s: list[str] = []
    if "comment" in el.attrib:
        s.append(el.attrib["comment"])
    if "since" in el.attrib:
        s.append("since " + el.attrib["since"])
    out = ", ".join(s)
    return " " + out if out != "" else ""


def fetch_union_name(el: ET.Element) -> str:
    out: list[str] = []
    for i, child in enumerate(el, start=1):
        if "name" in child.attrib:
            out.append(child.attrib["name"])
        else:
            out.append(f"unk_{i}")
    return "_or_".join(out)


def declare_prefix(name: str, default: str = "") -> str:
    if name in df_global:
        return "df.global."
    if name in df:
        return "df."
    return default


def parse_items_place() -> None:
    for file in sorted(Path(PATH_XML).glob("*.xml")):
        with file.open("r", encoding="utf-8") as src:
            data = src.read()
            for match in re.finditer(r"(struct|class|enum)-type.*type-name='([^']+).*'", data, re.MULTILINE):
                df.append(match.group(2))
            for match in re.finditer(r"(global-object).*\sname='([^']+).*'", data, re.MULTILINE):
                df_global.append(match.group(2))


def symbols_processing() -> None:
    print("Processing symbols...")
    parse_items_place()
    if not Path(PATH_OUTPUT).is_dir():
        Path(PATH_OUTPUT).mkdir(parents=True, exist_ok=True)
        with Path(PATH_LIB_CONFIG).open("w", encoding="utf-8") as dest:
            print(LIB_CONFIG, file=dest)
        with Path(PATH_CONFIG).open("w", encoding="utf-8") as dest:
            print(CONFIG, file=dest)

    for file in sorted(Path(PATH_XML).glob("*.xml")):
        print(f"Symbols -> {file.name}")
        with Path(f"{PATH_OUTPUT}{file.name.replace('.xml', '.lua')}").open("w", encoding="utf-8") as dest:
            print(parse_xml(Path(file)), file=dest)


########################################
#        Signatures processing         #
########################################


PATH_LUAAPI = "./library/LuaApi.cpp"
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
PATTERN_SIGNATURE = r"^([\w::<>]+).+?\(([^)]*)\)"
PATTERN_SIGNATURE_SEARCH = r"^.+[\s\*]+(DFHack::){0,1}module::function[\s]{0,1}\([\n]{0,1}(.|,\n)*?\n{0,1}\)[\s\n\w]+\{"


@dataclass
class Arg:
    name: str
    type: str
    default_value: Any | None
    unknown: bool = False


@dataclass
class Ret:
    type: str
    unknown: bool = False


@dataclass
class Signature:
    ret: Ret
    args: list[Arg]


@dataclass
class Entry:
    module: str
    fn: str
    type: str
    sig: str | None
    decoded_sig: Signature | None


def parse_luaapi() -> Iterable[Entry]:
    total = 0
    found = 0
    decoded = 0
    with Path(PATH_LUAAPI).open("r", encoding="utf-8") as file:
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
        args_pairs = match.group(2).split(", ")
        args: list[Arg] = []
        if args_pairs.__len__() > 0 and match.group(2).__len__() > 0:
            for arg_pair in args_pairs:
                default_value = None
                is_default = arg_pair.split("=")
                if is_default.__len__() > 1:
                    default_value = is_default[1].strip()
                    arg_pair = is_default[0].strip()
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
                )
                args.append(arg)
        if decoded_type_ret:
            return Signature(
                Ret(
                    type=decoded_type_ret.replace("::", "__").replace("enums__biome_type__", ""),
                    unknown=decoded_type_ret == type_ret and type_ret != "string",
                ),
                args,
            )
    return None


def print_entry(entry: Entry, prefix: str = "dfhack.") -> str:
    s = f"-- CXX SIGNATURE -> {entry.sig}\n"
    known_args = ""
    ret = ""
    if entry.decoded_sig:
        for arg in entry.decoded_sig.args:
            known_args += f"---@param {arg.name}{'?' if arg.default_value else ''} {arg.type}{' -- unknown' if arg.unknown else ''}{' -- default value is ' + arg.default_value if arg.default_value else ''}\n"
        ret = f"---@return {entry.decoded_sig.ret.type}{' -- unknown' if entry.decoded_sig.ret.unknown else ''}\n"
        s += known_args + ret
        s += f"function {prefix}{entry.module.lower()}{'.' if entry.module != '' else ''}{entry.fn}({', '.join([x.name for x in entry.decoded_sig.args])}) end\n"
    return s


dfhack_modules: list[str] = []


def generate_base_methods() -> None:
    with Path(PATH_DEFAULT).open("w", encoding="utf-8") as dest:
        out = f"-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n---@class df\ndf = nil\n---@class global\ndf.global = nil\n\n---@class dfhack\ndfhack = nil\n"
        for module in dfhack_modules:
            out += f"\n---@class {module.lower()}\ndfhack.{module.lower()} = nil\n"
        out += f"\n---@alias env {{ df: df, dfhack: dfhack }}\n\n---@param module string\n---@return env\nfunction mkmodule(module) end\n\n"
        print(out, file=dest)


def signatures_processing() -> None:
    print("Processing signatures...")
    with Path(PATH_DFHACK_OUTPUT).open("w", encoding="utf-8") as dest:
        print("-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n\n---@meta\n\n", file=dest)
        unknown_types: set[str] = set()
        for entry in parse_luaapi():
            if entry.decoded_sig:
                if entry.module and entry.module not in dfhack_modules:
                    dfhack_modules.append(entry.module)
                if entry.decoded_sig.ret.unknown:
                    unknown_types.add(f"---@alias {entry.decoded_sig.ret.type.replace('[]', '')} unknown\n")
                for arg in entry.decoded_sig.args:
                    if arg.unknown:
                        unknown_types.add(f"---@alias {arg.type.replace('[]', '')} unknown\n")
            print(print_entry(entry), file=dest)
        print(f"\n-- Unknown types\n{''.join(unknown_types)}", file=dest)

    generate_base_methods()


if __name__ == "__main__":
    symbols_processing()
    signatures_processing()
