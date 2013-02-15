local _ENV = mkmodule('plugins.dfusion.friendship')
local dfu=require("plugins.dfusion")
local ms=require("memscan")

local MAX_RACES=100
local MAX_CODE_DIST=250
FriendshipRainbow=defclass(FriendshipRainbow,dfu.BinaryPlugin)
FriendshipRainbow.name="FriendshipRainbow"
-- os independant... I think...
FriendshipRainbow.ATTRS{filename="hack/lua/plugins/dfusion/friendship.o",name="FriendshipRainbow",race_data=DEFAULT_NIL}
FriendshipRainbow.class_status="valid, not installed"
function FriendshipRainbow:findall_needles(codesg,needle) -- todo move to memscan.lua
    local cidx,caddr=codesg.uint8_t:find(needle)
    local ret={}
    while cidx~=nil do
       table.insert(ret,{cidx,caddr}) 
       cidx,caddr=codesg.uint8_t:find(needle,cidx+1)
    end
    return ret
end
function FriendshipRainbow:find_one(codesg,needle,crace)
    dfu.concatTables(needle,dfu.dwordToTable(crace))
    return self:findall_needles(codesg,needle)
end
function FriendshipRainbow:find_all()
    local code=ms.get_code_segment() 
    local locations={}
    local _,crace=df.sizeof(df.global.ui:_field("race_id"))
    
    dfu.concatTables(locations,self:find_one(code,{0x66,0xa1},crace)) --mov ax,[ptr]
    dfu.concatTables(locations,self:find_one(code,{0xa1},crace)) --mov ax,[ptr]
    local registers=
    {0x05, -- (e)ax
    0x1d, --ebx
    0x0d, --ecx
    0x15, --edx
    0x35, --esi
    0x3d, --edi
    --0x25, --esp not used?
    --0x2d, --ebp not used?
    }
    for k,reg in ipairs(registers) do
        
        dfu.concatTables(locations,self:find_one(code,{0x0f,0xbf,reg},crace)) --movsx reg,[ptr]
        dfu.concatTables(locations,self:find_one(code,{0x66,0x8b,reg},crace)) --mov reg,[ptr]
    end
   
    return self:filter_locations(code,locations)
end
function FriendshipRainbow:filter_locations(codesg,locations)
    local ret={}
    local registers={0x80,0x83,0x81,0x82,0x86,0x87,
                     0x98,0x9b,0x99,0x9a,0x9e,0x9f,
                     0x88,0x8b,0x89,0x8a,0x8e,0x8f,
                     0x90,0x93,0x91,0x92,0x96,0x97,
                     0xb0,0xb3,0xb1,0xb2,0xb6,0xb7,
                     0xb8,0xbb,0xb9,0xba,0xbe,0xbf}
    for _,entry in ipairs(locations) do
        for _,r in ipairs(registers) do
            
            local idx,addr=codesg.uint8_t:find({0x39,r,0x8c,0x00,0x00,0x00},
                codesg.uint8_t:addr2idx(entry[2]),codesg.uint8_t:addr2idx(entry[2])+MAX_CODE_DIST)
            if addr then
                table.insert(ret,{addr,r})
                break
            end
            idx,addr=codesg.uint8_t:find({0x3b,r,0x8c,0x00,0x00,0x00},
                codesg.uint8_t:addr2idx(entry[2]),codesg.uint8_t:addr2idx(entry[2])+MAX_CODE_DIST)
            if addr then
                table.insert(ret,{addr,r})
                break
            end
        end
    end
    return ret
end
function FriendshipRainbow:patchCalls(target)
    local addrs=self:find_all()
    local swaps={}
    for k,adr in ipairs(addrs) do
        local newval=dfu.makeCall(adr[1],target)
        table.insert(newval,adr[2])
        for t,val in ipairs(newval) do
            swaps[adr[1]+t-1]=val
        end
    end
    dfhack.internal.patchBytes(swaps)
end
function FriendshipRainbow:set_races(arr)
    local n_to_id=require("plugins.dfusion.tools").build_race_names()
    local ids={}
    for k,v in ipairs(self.race_data) do -- to check if all races are valid.
        ids[k]=n_to_id[v] 
    end
    for k,v in ipairs(ids) do
        arr[k-1]=ids[k]
    end
end
function FriendshipRainbow:install(races)
        self.race_data=races or self.race_data
        if #self.race_data<1 then
            error("race count must be bigger than 0")
        end
        if #self.race_data>MAX_RACES then
            error("race count must be less then "..MAX_RACES)
        end
        local rarr=self:allocate("race_array",'uint16_t',MAX_RACES)
        local _,rarr_offset=df.sizeof(rarr)
        self:set_marker_dword("racepointer",rarr_offset)
        self:set_races(rarr)
        self:set_marker_dword("racecount",#self.race_data)
        local safe_loc=self:allocate("safe_loc",'uint32_t',1)
        local _1,safe_loc_offset=df.sizeof(safe_loc)
        self:set_marker_dword("safeloc1",safe_loc_offset)
        self:set_marker_dword("safeloc2",safe_loc_offset)
        local addr=self:move_to_df()
        self:patchCalls(addr)
        self.installed=true
end
Friendship=Friendship or FriendshipRainbow()
return _ENV