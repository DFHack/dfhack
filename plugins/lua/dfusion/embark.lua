local _ENV = mkmodule('plugins.dfusion.embark')
local dfu=require("plugins.dfusion")
local ms=require("memscan")
local MAX_RACES=100
CustomEmbark=defclass(CustomEmbark,dfu.BinaryPlugin)
local myos=dfhack.getOSType()
if myos=="windows" then
    CustomEmbark.ATTRS{filename="hack/lua/plugins/dfusion/embark.o",name="CustomEmbark",race_caste_data=DEFAULT_NIL}
    function CustomEmbark:parseRaces(races)
        if #races<7 then
            error("caste and race count must be bigger than 6")
        end
        if #races>MAX_RACES then
            error("caste and race count must be less then "..MAX_RACES)
        end
        local n_to_id=require("plugins.dfusion.tools").build_race_names()
       
        local ids={}
        for k,v in pairs(races) do
            local race=v[1] or v
            ids[k]={}
            ids[k][1]=n_to_id[race]
            if ids[k][1]==nil then qerror(race.." not found!") end
            ids[k][2]=v[2] or -1
        end
        self.race_caste_data=ids
    end
    function CustomEmbark:install(race_caste_data)
        local stoff=dfhack.internal.getAddress('start_dwarf_count')
        if race_caste_data~=nil then
            self:parseRaces(race_caste_data)
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
        self.call_patch=self.call_patch or dfu.BinaryPatch{pre_data=needle,data=call_data,address=trg_offset,name="custom_embark_call_patch"}
        needle={0x83,0xc8,0xff} -- or eax, 0xFF
        local _,caste_offset=mem.uint8_t:find(needle,mem.uint8_t:addr2idx(trg_offset),nil)
        if caste_offset==nil or caste_offset-stoff>1000 then
            error("Caste change code not found or found too far!")
        end
        
        self.disable_castes=self.disable_castes or dfu.BinaryPatch{pre_data={0x83,0xc8,0xff},data={0x90,0x90,0x90},address=caste_offset,name="custom_embark_caste_disable"}
        self.disable_castes:apply()
        
        
        self:setEmbarkParty(self.race_caste_data)
        local caste_array=self:get_or_alloc("caste_array","uint16_t",MAX_RACES)
        local race_array=self:get_or_alloc("race_array","uint16_t",MAX_RACES)
        
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
    
    function CustomEmbark:setEmbarkParty(racesAndCastes)
        local stoff=dfhack.internal.getAddress('start_dwarf_count')
        
        
        if self.dwarfcount== nil then
            self.dwarfcount=dfu.BinaryPatch{pre_data=dfu.dwordToTable(7),data=dfu.dwordToTable(#self.race_caste_data),address=stoff,name="custom_embark_embarkcount"}
            self.dwarfcount:apply()
        else
            self.dwarfcount:repatch(dfu.dwordToTable(#self.race_caste_data))
        end
        local caste_array=self:get_or_alloc("caste_array","uint16_t",MAX_RACES)
        local race_array=self:get_or_alloc("race_array","uint16_t",MAX_RACES)
        
        for k,v in ipairs(self.race_caste_data) do
            caste_array[k-1]=v[2] or -1
            race_array[k-1]=v[1]
        end
    end
    function CustomEmbark:status()
        if self.installed then
            return "valid, installed"
        else
            return "valid, not installed"
        end
    end
    function CustomEmbark:uninstall()
        if self.installed then
            self.call_patch:remove()
            self.disable_castes:remove()
            self.dwarfcount:remove()
        end
    end
    function CustomEmbark:unload()
        self:uninstall()
        if Embark~=nil then
            Embark=nil
        end
    end
    Embark=Embark or CustomEmbark()
else
    CustomEmbark.status=function() return"invalid, os not supported" end
end
return _ENV