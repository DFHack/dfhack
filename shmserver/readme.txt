Using the shm server:
copy files to DF/libs folder
g++ -fPIC -c dfconnect.c -o dfconnect.o
g++ -shared -o dfconnect.so dfconnect.o -ldl

edit DF/df script and add this line just before DF is called:
export LD_PRELOAD="./libs/dfconnect.so" # Hack DF!

save and run the script!

Has to be compiled for 32bit arch, otherwise the library isn't recognised. Client can be any arch.

Precompiled dfconnect library is made available. dfconnect.so goes in DF/libs, df-hacked script goes in DF/
Run ./df-hacked to use the shared memory API