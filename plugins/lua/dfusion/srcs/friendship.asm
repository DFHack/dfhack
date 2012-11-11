.intel_syntax
push eax
mov eax,[esp+0x04]
push ebx
pushfd
mov eax,[eax] # get a byte after the call this procedure to analyze what register holds cr ptr
jmptbl:
cmp al,0x81
jz regC
cmp al,0x82
jz regD
cmp al,0x83
jz regB
cmp al,0x85
jz regBP
cmp al,0x86
jz regESI
cmp al,0x87
jz regEDI
cmp al,0x88
jz regA
cmp al,0x8A
jz regD
cmp al,0x8B
jz regB
cmp al,0x8D
jz regBP
cmp al,0x8E
jz regESI
cmp al,0x8F
jz regEDI
cmp al,0x90
jz regA
cmp al,0x91
jz regC
cmp al,0x93
jz regB
cmp al,0x95
jz regBP
cmp al,0x96
jz regESI
cmp al,0x97
jz regEDI
jmp fail
regA:
mov eax, [esp+0x8]
mov eax, [eax+0x8c]
jmp compare
regC:
mov eax, [ecx+0x8c]
jmp compare
regB:
mov eax, [ebx+0x8c]
jmp compare
regD:
mov eax, [edx+0x8c]
jmp compare
regBP:
mov eax, [ebp+0x8c]
jmp compare
regESI:
mov eax, [esi+0x8c]
jmp compare
regEDI:
mov eax, [edi+0x8c]
#jmp compare
compare:
push ecx
mark_racepointer:
mov ebx,0xDEADBEEF #write a pointer to the list of allowed races
mark_racecount:
mov ecx,0xDEADBEEF #write a number of allowed races
loop1:
cmp word[ebx+ecx*2],ax
jz endok
dec ecx
cmp ecx ,-1
jnz loop1
pop ecx
popfd
jmp fail
endok:
pop ecx
popfd
cmp eax,eax
jmp endfinal
fail:

xor ebx,ebx
xor eax,eax
inc eax
cmp eax,ebx
endfinal:

pop ebx
pop eax
mark_safeloc1:
mov [0xDEADBEEF],eax #write a pointer to safe location (usually after this)
pop eax
pushfd
inc eax #skip one instruction
popfd
push eax
mark_safeloc2:
mov eax,[0xDEADBEEF] #write a pointer to safe location (same as above)
ret
