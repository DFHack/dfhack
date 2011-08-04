.intel_syntax
mov eax , [esp+0x1C]
caste:
movsx eax, word ptr[eax*2+0xdeadbeef]
mov [esp+0x04],eax
mov eax , [esp+0x1C]
race:
movzx eax,word ptr [eax*2+0xDEADBEEF]
ret
