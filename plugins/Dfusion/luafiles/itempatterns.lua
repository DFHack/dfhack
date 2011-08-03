--dofile("patterns2.lua") moved to common.lua
ptr_item={}
ptr_item.RTI={off=0,rtype=DWORD}
ptr_item.x={off=4,rtype=WORD}
ptr_item.y={off=6,rtype=WORD}
ptr_item.z={off=8,rtype=WORD}
ptr_item.ref={off=0x28,rtype=ptr_vector} 

ptr_item.mat={off=0x78,rtype=WORD} 
ptr_item.submat={off=0x7A,rtype=WORD} 
ptr_item.submat2={off=0x7C,rtype=DWORD}
ptr_item.legendid={off=0x80,rtype=DWORD} -- i don't remember writing this...
ptr_item.decorations={off=0x90,rtype=ptr_vector} 
ptr_item.flags={off=0xC,rtype=ptt_dfflag.new(8)} 
ptr_item.ptr_covering={off=0x64,rtype=DWORD}
ptr_item.stack={off=0x58,rtype=WORD}
function ptr_item.getname(self,RTI) 
	if RTI == nil then
		return string.sub(RTTI_GetName(self.RTI),5,-3)
	else
		return string.sub(RTTI_GetName(RTI),5,-3)
	end
end
ptr_subitems={}
ptr_subitems["item_slabst"]={}
ptr_subitems["item_slabst"].msgptr={off=0xA0,rtype=ptt_dfstring}
ptr_subitems["item_slabst"].signtype={off=0xC0,rtype=DWORD}

ptr_subitems["item_fisthst"]={}
ptr_subitems["item_fisthst"].fisthtype={off=0x78,rtype=WORD}

ptr_subitems["item_eggst"]={}
ptr_subitems["item_eggst"].race={off=0x78,rtype=DWORD}
ptr_subitems["item_eggst"].isfertile={off=0xa0,rtype=DWORD} --0 or 1
ptr_subitems["item_eggst"].hatchtime={off=0xa4,rtype=DWORD}

ptr_decoration_gen={}
ptr_decoration_gen.RTI={off=0,rtype=DWORD}
ptr_decoration_gen.material={off=0x04,rtype=WORD} -- same for all?
ptr_decoration_gen.submat={off=0x08,rtype=DWORD}
function ptr_decoration_gen.getname(self,RTI) 
	if RTI == nil then
		return string.sub(RTTI_GetName(self.RTI),21,-5)
	else
		return string.sub(RTTI_GetName(RTI),21,-5)
	end
end
ptr_decoration={}
ptr_decoration["covered"]={}
ptr_decoration["covered"].material={off=0x04,rtype=WORD}
ptr_decoration["covered"].submat={off=0x08,rtype=DWORD}
ptr_decoration["art_image"]={}
ptr_decoration["art_image"].material={off=0x04,rtype=WORD}
ptr_decoration["art_image"].submat={off=0x08,rtype=DWORD}
ptr_decoration["art_image"].image={off=0x24,rtype=DWORD}
ptr_decoration["bands"]={}
ptr_decoration["bands"].material={off=0x04,rtype=WORD}
ptr_decoration["bands"].submat={off=0x08,rtype=DWORD}
ptr_cover={} --covering of various types (blood, water, etc)
ptr_cover.mat={off=0,rtype=WORD}
ptr_cover.submat={off=4,rtype=DWORD}
ptr_cover.state={off=8,rtype=WORD}