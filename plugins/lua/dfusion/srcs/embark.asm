.intel_syntax
mov eax , [esp+0x1C] # loop counter
mark_caste:
movsx ecx, word ptr[eax*2+0xdeadbeef]
mark_race:
movzx eax,word ptr [eax*2+0xDEADBEEF]
ret
