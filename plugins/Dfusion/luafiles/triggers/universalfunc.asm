.intel_syntax
#eax - num of function
#ebx -start of data
mov ecx,eax
mul 10
add ebx
mov ebx,[eax+4]
mov ecx,[eax+8]
mov edx,[eax+12]
mov esi,[eax+16]
mov edi,[eax+20]
mov eax,[eax]

retn
