.intel_syntax
eaxpart:
push eax
push ecx
jmp compare
ecxpart:
push eax
push ecx
mov eax,ecx

compare:
push ebx
mov ebx,0xDEADBEEF #write a pointer to the list of allowed civs
mov ecx,2000 #write a number of allowed civs
loop1:
cmp [ebx+ecx*4],eax
jnz endok
dec ecx
cmp ecx ,-1
jnz loop1

pop ebx

jmp fail

endok:
pop ebx

cmp eax,eax
jmp endfinal
fail:

xor ecx,ecx
xor eax,eax
inc eax
cmp eax,ebx
endfinal:

pop ecx
pop eax
ret