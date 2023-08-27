import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

PATH_LUAAPI = "./library/LuaApi.cpp"
PATH_LIBRARY = "./library/"

PATTERN_WRAPM = r"WRAPM\((.+), (.+)\)"
PATTERN_CWRAP = r"CWRAP\((.+), (.+)\)"
PATTERN_WRAPN = r"WRAPN\((.+), (.+)\)"
PATTERN_MODULE_ARRAY = r"dfhack_\w+_module\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ARRAY = r"dfhack_\w+_funcs\[\](.|\n)*?NULL,\s{0,1}NULL\s{0,1}\}\n\}"
PATTERN_LFUNC_ITEM = r"\{\s\"(\w+)\",\s(\w+)\s\}"
PATTERN_SIGNATURE = r"^([\w::]+).+?\((.*)?\)"
PATTERN_SIGNATURE_SEARCH = r"^.+[\s\*]+(DFHack::){0,1}module::function[\s]{0,1}\([\n]{0,1}.*?\)[\s\n\w]+\{"


@dataclass
class Arg:
    name: str
    type: str
    unknown: bool = False


@dataclass
class Signature:
    ret: str
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
        wrappers = [WRAPM, WRAPN, CWRAP, LFUN]

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
                            # print(item)
                            yield item

    print(f"Signatures -> total: {total}, found: {found}, decoded {decoded}")


def LFUN(array: str) -> Iterable[Entry]:
    module = array.split("_")[1].capitalize()
    for match in re.finditer(PATTERN_LFUNC_ITEM, array):
        item = Entry(module, match.group(1), "LFUNC", None, None)
        item.sig = find_signature(item)
        yield item


def CWRAP(array: str) -> Iterable[Entry]:
    module = array.split("_")[1].capitalize()
    for match in re.finditer(PATTERN_CWRAP, array):
        item = Entry(module, match.group(1), "CWRAP", None, None)
        item.sig = find_signature(item)
        yield item


def WRAPM(array: str) -> Iterable[Entry]:
    for match in re.finditer(PATTERN_WRAPM, array):
        if any(c in match.group(1) or c in match.group(2) for c in ["{", "}"]):
            print("SKIP", match.group(0))
            continue
        item = Entry(match.group(1), match.group(2), "WRAPM", None, None)
        item.sig = find_signature(item)
        yield item


def WRAPN(array: str) -> Iterable[Entry]:
    module = array.split("_")[1].capitalize()
    for match in re.finditer(PATTERN_WRAPN, array):
        if any(c in match.group(1) or c in match.group(2) for c in ["{", "}"]):
            print("SKIP", match.group(0))
            continue
        item = Entry(module, match.group(1), "WRAPN", None, None)
        item.sig = find_signature(item)
        yield item


def find_signature(item: Entry) -> str | None:
    for entry in Path(PATH_LIBRARY).rglob("*.cpp"):
        with entry.open("r", encoding="utf-8") as file:
            data = file.read()
            r = PATTERN_SIGNATURE_SEARCH.replace("module", item.module).replace("function", item.fn)
            match = re.search(r, data, re.MULTILINE)
            if match:
                sig = match.group(0).replace("\n", "").replace("{", "").replace("DFHACK_EXPORT ", "").strip()
                if sig.startswith("if (") or sig.startswith("<<"):
                    return None
                return sig


def decode_type(cxx: str) -> str:
    match cxx:
        case "int" | "int8_t" | "uint8_t" | "int16_t" | "uint16_t" | "int32_t" | "uint32_t" | "int64_t" | "uint64_t" | "size_t":
            return "integer"
        case "float" | "long" | "ulong":
            return "number"
        case "bool":
            return "boolean"
        case "string" | "std::string" | "char":
            return "string"
        case "void":
            return "nil"
        case _:
            if cxx.startswith("std::vector<"):
                return decode_type(cxx.replace("std::vector<", "")[:-1]).strip() + "[]"
            if cxx.startswith("df::"):
                return cxx[4:]
            # print("UNKNOWN TYPE", cxx)
            return cxx


def decode_signature(sig: str) -> Signature | None:
    # print(sig)
    for k in ["const ", "*", "&"]:
        sig = sig.replace(k, "")
    match = re.search(PATTERN_SIGNATURE, sig, re.MULTILINE)
    if match:
        ret = decode_type(match.group(1))
        # print("RET TYPE", decode_type(match.group(1)))
        args_pairs = match.group(2).split(", ")
        args: list[Arg] = []
        if args_pairs.__len__() > 0 and match.group(2).__len__() > 0:
            for arg_pair in args_pairs:
                arg_name = arg_pair.split(" ")[-1]
                arg_type = arg_pair.replace(" " + arg_name, "").strip()
                decoded_type = decode_type(arg_type)
                arg = Arg(arg_name, decoded_type, decoded_type == arg_type)
                args.append(arg)
                # print(arg)
        if ret:
            return Signature(ret, args)
    return None


def print_entry(entry: Entry, prefix: str = "dfhack.") -> str:
    s = f"-- CXX SIGNATURE -> {entry.sig}\n"
    unknown_args = ""
    known_args = ""
    ret = ""
    if entry.decoded_sig:
        for arg in entry.decoded_sig.args:
            if arg.unknown:
                unknown_args += f"---@alias {arg.type} unknown\n"
            known_args += f"---@param {arg.name} {arg.type}\n"
        ret = f"---@return {entry.decoded_sig.ret}\n"
        s += unknown_args + known_args + ret
        s += f"function {prefix}{entry.module.lower()}.{entry.fn}({', '.join([x.name for x in entry.decoded_sig.args])}) end\n"
    return s


if __name__ == "__main__":
    for entry in parse_luaapi():
        s = print_entry(entry)
        print(s)
