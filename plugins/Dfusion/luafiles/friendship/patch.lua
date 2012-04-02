function friendship_in.patch()
	UpdateRanges()
	pos=GetTextRegion().start
	local _,crace=df.sizeof(df.global.ui:_field("race_id"))
	hits={}
	i=1
	repeat
		--todo make something better/smarter...
		pos1=offsets.find(pos+7,0x0f,0xBF,ANYBYTE,DWORD_,crace) -- movsx 
		pos2=offsets.find(pos+7,0x66,0xa1,DWORD_,crace) -- mov ax,[ptr]
		pos3=offsets.find(pos+7,0xa1,DWORD_,crace) -- mov eax,[ptr]
		pos4=offsets.find(pos+7,0x66,0x8b,ANYBYTE,DWORD_,crace) -- mov ANYREG,[ptr]
		--pos5=offsets.find(pos+7,0x66,0x8b,0x15,DWORD_,crace) -- mov dx,[ptr]
		pos=minEx{pos1,pos2,pos3,pos4}
		if pos ~=0 then 
			hits[i]=pos
			i=i+1
			print(string.format("Found at %x",pos)) 
		end
	until pos==0
	print("=======================================")
	for _,p in pairs(hits) do
		myp=p
		repeat

			--print(string.format("Analyzing %x...",p))
			--TODO read offset from memory.xml
			pos1=offsets.find(myp,0x39,ANYBYTE,0x8c,00,00,00) -- compare [reg+08c] (creature race) with any reg 
			pos2=offsets.find(myp,0x3b,ANYBYTE,0x8c,00,00,00) -- compare  any reg with [reg+08c] (creature race)
			pos=minEx{pos1,pos2}
			if pos ~=0 then
				
				if(pos-p>250) then
					--this here does not work yet...
					--[[pos =offsets.find(p,CALL)
					print(string.format("Distance to call:%x",pos-p))
					print(string.format("Call: %x",signDword(engine.peekd(pos+1)+pos)))
					pos=analyzeF(signDword(signDword(engine.peekd(pos+1)+pos)))
					
					print(string.format("Cmp @:%x",pos))]]--
					print(string.format("skipping %x... Cmp too far away (dist=%i)",p,pos-p))
				else
					--print(string.format("Found at %x, simple compare",pos)) 
					--print(string.format("Distance =%x",pos-p))
					--patch compares
					
					pokeCall(pos)
				end
			else
				break
			end
			myp=myp+pos+6
			if myp-p >250 then break end
		until false

	end
end