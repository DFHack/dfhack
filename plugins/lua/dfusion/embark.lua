local _ENV = mkmodule('plugins.dfusion.embark')
local dfu=require("plugins.dfusion")
local ms=require("memscan")
local MAX_RACES=100
CustomEmbark=defclass(CustomEmbark,dfu.BinaryPlugin)
CustomEmbark.name="CustomEmbark"
local myos=dfhack.getOSType()
if myos=="windows" then
    CustomEmbark.ATTRS{filename="hack/lua/plugins/dfusion/embark.o",name="CustomEmbark",race_caste_data=DEFAULT_NIL}
    CustomEmbark.class_status="valid, not installed"
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
        if #racesAndCastes<7 then
            error("caste and race count must be bigger than 6")
        end
        if #racesAndCastes>MAX_RACES then
            error("caste and race count must be less then "..MAX_RACES)
        end
        
        self.race_caste_data=racesAndCastes
        if self.dwarfcount== nil then
            self.dwarfcount=dfu.BinaryPatch{pre_data=dfu.dwordToTable(7),data=dfu.dwordToTable(#self.race_caste_data),address=stoff,name="custom_embark_embarkcount"}
            self.dwarfcount:apply()
        else
            self.dwarfcount:repatch(dfu.dwordToTable(#self.race_caste_data))
        end
        local caste_array=self:get_or_alloc("caste_array","uint16_t",MAX_RACES)
        local race_array=self:get_or_alloc("race_array","uint16_t",MAX_RACES)
        for k,v in ipairs(self.race_caste_data) do
            caste_array[k-1]=v[2]
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
    function CustomEmbark:edit()
        local data=self.race_caste_data or {}
        print(string.format("Old race count:%d",#data))
        local endthis=false
        print("current:")
        while(not endthis) do
            print(" #  RaceId       Race name        Caste num")
            for k,v in pairs(data) do
                local name=df.creature_raw.find(v[1]).creature_id or ""
                print(string.format("%3d. %6d  %20s %d",k,v[1],name,v[2]))
            end
            print("a- add, r-remove, c-cancel, s-save and update")
            local choice=io.stdin:read()
            if choice=='a' then
                print("Enter new race then caste ids:")
                local race=tonumber(io.stdin:read())
                local caste=tonumber(io.stdin:read())
                if race and caste then
                    table.insert(data,{race,caste})
                else
                    print("input parse error")
                end
            elseif choice=='r' then
                print("enter number to remove:")
                local num_rem=tonumber(io.stdin:read())
                if num_rem~=nil then
                    table.remove(data,num_rem)
                end
            elseif choice=='c' then
                endthis=true
            elseif choice=='s' then
                endthis=true
                if self.installed then
                    self:setEmbarkParty(data)
                else
                    self.race_caste_data=data
                    self:install()
                end
            end
        end
    end
else
    CustomEmbark.class_status="invalid, os not supported"
end
return _ENV