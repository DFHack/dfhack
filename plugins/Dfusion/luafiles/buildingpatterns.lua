ptr_building={}
ptr_building.RTI={off=0,rtype=DWORD}
ptr_building.xs={off=4,rtype=DWORD}
ptr_building.ys={off=6,rtype=DWORD}
ptr_building.zs={off=8,rtype=DWORD}
ptr_building.xe={off=12,rtype=DWORD}
ptr_building.ye={off=16,rtype=DWORD}
ptr_building.ze={off=20,rtype=DWORD}
ptr_building.flags={off=24,rtype=ptt_dfflag.new(4)}
ptr_building.materials={off=28,rtype=DWORD}
ptr_building.builditems={off=228,rtype=ptr_vector}
function ptr_building.getname(self,RTI) 
	if RTI == nil then
		return string.sub(RTTI_GetName(self.RTI),5,-3)
	else
		return string.sub(RTTI_GetName(RTI),5,-3)
	end
end
ptr_subbuilding={}
ptr_subbuilding["building_trapst"]={}
ptr_subbuilding["building_trapst"].state={off=250,rtype=DWORD} -- atleast lever has this
ptr_subbuilding["building_doorst"]={}
ptr_subbuilding["building_doorst"].flg={off=248,rtype=WORD} --maybe flags?
ptr_subbuilding["building_doorst"].state={off=250,rtype=DWORD}
