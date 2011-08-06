.intel_syntax
nop #5 nops for instruction thats replaced by call
nop
nop
nop
nop
pushad
pushfd
saveplace31:
mov [0xDEADBEEF], esp
trigercount:
mov eax, [0xDEADBEEF] #mov count of triggers.
f_loc:
mov esi, 0xdeadbeef #mov location of functions.
f_data:
mov ebx, 0xDEADBEEF #mov a start of function data
test eax,eax
jz lend
lstart:
dec eax
push ebx
push eax

mov eax,[esi+eax*4]
saveplace:
mov [0xDEADBEEF],eax #save function for later
pop eax
push eax
mov edx,44
mul edx
add eax,ebx
#stack preparation
mov ebx,[eax+24]
push ebx
mov ebx,[eax+28]
push ebx
mov ebx,[eax+32]
push ebx
mov ebx,[eax+36]
push ebx
mov ebx,[eax+40]
push ebx
mov ebx,[eax+4]
mov ecx,[eax+8]
mov edx,[eax+12]
mov esi,[eax+16]
mov edi,[eax+20]
mov eax,[eax]
saveplace2:
call [0xdeadbeef]  #same save loc
results:
mov [0xDEADBEEF],eax #get result
saveplace33:
mov esp, [0xDEADBEEF]
add esp, -8
pop eax
pop ebx
cmp eax,0
jnz lstart
lend:
xor eax,eax
trigcount2:
mov  dword ptr [0xDEADBEEF],  eax # zero triggers
saveplace32:
mov esp, [0xDEADBEEF]
popfd
popad
ret
