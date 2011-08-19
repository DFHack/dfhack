.intel_syntax
push eax
push edx
push ecx
push ebx
push eax
mov eax,[esp+24]
push eax
function:
call 0xdeadbeef
function2:
mov [0xdeadbeef],eax #self modifying code... :/
pop eax
function3:
call [0xdeadbeef]




