local dfu=require("plugins.dfusion")
local ms=require("memscan")
local MAX_RACES=100
CustomEmbark=defclass(CustomEmbark,dfu.BinaryPlugin)
CustomEmbark.ATTRS{filename="dfusion/embark/embark.o",name="CustomEmbark",race_caste_data=DEFAULT_NIL}
function CustomEmbark:install()
    local stoff=dfhack.internal.getAddress('start_dwarf_count')
    if #self.race_caste_data<7 then
        error("caste and race count must be bigger than 6")
    end
    if #self.race_caste_data>MAX_RACES then
        error("caste and race count must be less then "..MAX_RACES)
    end
    if stoff==nil then
        error("address for start_dwarf_count not found!")
    end
    local _,race_id_offset=df.sizeof(df.global.ui:_field("race_id"))
    print(string.format("start=%08x",stoff))
    local needle={0x0f,0xb7,0x0d} --movzx eax,dword ptr [race_id]
    local tmp_table=dfu.dwordToTable(race_id_offset)
    for k,v in ipairs(tmp_table) do
        table.insert(needle,v)
    end
    
    local mem=ms.get_code_segment() 
    print(mem.uint8_t:addr2idx(stoff))
    print(mem.uint8_t:find(needle,mem.uint8_t:addr2idx(stoff)))
    local _,trg_offset=mem.uint8_t:find(needle,mem.uint8_t:addr2idx(stoff),nil)--maybe endoff=stoff+bignumber
    if trg_offset==nil then
        error("address for race_load not found")
    end
    local call_data={0x90,0x90}
    local _,data_offset=df.sizeof(self.data)
    dfu.concatTables(call_data,dfu.makeCall(trg_offset+2,data_offset))
    self.call_patch=dfu.BinaryPatch{pre_data=needle,data=call_data,address=trg_offset,name="custom_embark_call_patch"}
    needle={0x83,0xc8,0xff} -- or eax, 0xFF
    local _,caste_offset=mem.uint8_t:find(needle,mem.uint8_t:addr2idx(trg_offset),nil)
    if caste_offset==nil or caste_offset-stoff>1000 then
        error("Caste change code not found or found too far!")
    end
    
    self.disable_castes=dfu.BinaryPatch{pre_data={0x83,0xc8,0xff},data={0x90,0x90,0x90},address=caste_offset,name="custom_embark_caste_disable"}
    self.disable_castes:apply()
    self.dwarfcount=dfu.BinaryPatch{pre_data=dfu.dwordToTable(7),data=dfu.dwordToTable(#self.race_caste_data),address=stoff,name="custom_embark_embarkcount"}
    self.dwarfcount:apply()
    local caste_array=self:allocate("caste_array","uint16_t",#self.race_caste_data)
    local race_array=self:allocate("race_array","uint16_t",#self.race_caste_data)
    for k,v in ipairs(self.race_caste_data) do
        caste_array[k-1]=v[2]
        race_array[k-1]=v[1]
    end
    local race_array_off,caste_array_off
    local _
    _,race_array_off=df.sizeof(race_array)
    _,caste_array_off=df.sizeof(caste_array)
    self:set_marker_dword("race",caste_array_off) --hehe... mixed them up i guess...
    self:set_marker_dword("caste",race_array_off)
    
    self:move_to_df()
    self.call_patch:apply()
    self.installed=true
end
function CustomEmbark:uninstall()
    if self.installed then
        self.call_patch:remove()
        self.disable_castes:remove()
        self.dwarfcount:remove()
    end
end