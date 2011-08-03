ptr_COL={} -- complete object locator...
ptr_COL.sig={off=0,rtype=DWORD}
ptr_COL.offset={off=4,rtype=DWORD} --offset of this vtable in the complete class
ptr_COL.cdoffset={off=8,rtype=DWORD} -- constructor displacement
ptr_COL.typePointer={off=12,rtype=DWORD}
ptr_COL.hierarchyPointer={off=16,rtype=DWORD}

ptr_RTTI_Type={}
ptr_RTTI_Type.vftPointer={off=0,rtype=DWORD}
ptr_RTTI_Type.name={off=8,rtype=STD_STRING}

function RTTI_GetName(vtable)
	local COLoff=engine.peek(vtable-4,DWORD)
	--print(string.format("Look:%x vtable:%x",vtable,engine.peek(vtable-4,DWORD)))
	COL=engine.peek(COLoff,ptr_COL)
	--print(string.format("COL:%x Typeptr:%x Type:%s",COLoff,COL.typePointer,engine.peek(COL.typePointer,ptr_RTTI_Type.name)))
	return engine.peek(COL.typePointer,ptr_RTTI_Type.name)
end
ptr_RTTI_Hierarchy={}
ptr_RTTI_Hierarchy.sig={off=0,rtype=DWORD}
ptr_RTTI_Hierarchy.attributes={off=4,rtype=DWORD}
ptr_RTTI_Hierarchy.numBaseClasses={off=8,rtype=DWORD}
ptr_RTTI_Hierarchy.ptrBases={off=12,rtype=DWORD}

ptr_RTTI_BaseClass={}
ptr_RTTI_BaseClass.typePointer={off=0,rtype=DWORD}
ptr_RTTI_BaseClass.numContained={off=4,rtype=DWORD}
--todo PMD
--todo flags