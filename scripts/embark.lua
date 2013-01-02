-- lets you embark anywhere.
helpstring=[[ embark [-t|-d|-e|-h]
 -t   test if the patch can be applied
 -d   disable the patch if enabled
 -e   enable the patch if disabled
 -h   shows help
]]
args={...}
if args[1]=="-h" then
    print(helpstring)
    return
end
local ms=require("memscan")
local dfu=require("plugins.dfusion")
local patch
function embark() --windows only?
	local seg=ms.get_code_segment()
	local idx,off
    local patch=dfu.patches.embark_anywhere
	if patch~=nil then
       return patch
    else
        idx,off=seg.uint8_t:find_one{0x66, 0x83, 0x7F ,0x1A ,0xFF,0x74,0x04}
        if idx then
            patch=dfu.BinaryPatch{name="embark_anywhere",pre_data={0x74,0x04},data={0x90,0x90},address=off+5}
            return patch
        else
            qerror("Offset for embark patch not found!")
        end
    end
end
patch=embark()
if args[1]=="-t" then
    if patch:test() and not patch.is_applied then
        print("all okay, patch can be applied")
    elseif patch.is_applied then
        print("patch is currently applied, can be removed")
    else
        qerror("patch can't be applied")
    end
elseif args[1]=="-d" then
    if patch.is_applied then
        patch:remove()
        print("patch removed")
    end
elseif args[1]=="-e" then
    if not patch.is_applied then
        patch:apply()
        print("patch applied")
    end
else
    print(helpstring)
end