.intel_syntax
push eax
push ebp
push esp
push esi
push edi
push edx
push ecx
push ebx
push eax
mov eax,[esp+36]
push eax
function:
call 0xdeadbee0
function2:
mov [0xdeadbeef],eax
pop eax
function3:
jmp [0xdeadbeef]




