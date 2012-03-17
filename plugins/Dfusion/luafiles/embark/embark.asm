.intel_syntax
mov eax , [esp+0x1C]
caste:
movsx ecx, word ptr[eax*2+0xdeadbeef]
race:
movzx eax,word ptr [eax*2+0xDEADBEEF]
ret
