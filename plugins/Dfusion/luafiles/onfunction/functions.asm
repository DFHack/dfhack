.intel_syntax
push eax
push edx
push ecx
push ebx
push eax
mov eax,[esp+20]
push eax
function:
call 0xdeadbee4
function2:
mov [0xdeadbeef],eax #self modifying code... :/
pop eax
function3:
call [0xdeadbeef]




