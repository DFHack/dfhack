.intel_syntax
pushfd
push ebx
push edx
mov eax,[0xdeadbeef] # get old seed
mov ebx,1103515245
#mul 1103515245
mul ebx
add eax,12345
mov [0xdeadbeef],eax #put seed back...thus generation rnd is complete

xor edx,edx
mov ebx,2000 #put size of array here
div ebx #why oh why there is no div const? compiler prob makes some xor/add magic
movzx eax,word ptr[0xdeadbeef+edx*2]
pop edx
pop ebx

popfd
ret
