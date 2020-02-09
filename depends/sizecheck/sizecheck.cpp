// adapted from https://github.com/mifki/df-sizecheck/blob/master/b.cpp
// usage:
// linux: PRELOAD_LIB=hack/libsizecheck.so ./dfhack

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace std;

const uint32_t MAGIC = 0xdfdf4ac8;

void* alloc(size_t n) {
    void* addr;
    if (posix_memalign(&addr, 32, n + 16) != 0) {
        return addr;
    }
    memset(addr, 0, 16);
    *(size_t*)addr = n;
    *(uint32_t*)((uint8_t*)addr + 8) = MAGIC;
    return (uint8_t*)addr + 16;
}

void dealloc(void* addr) {
    if (intptr_t(addr) % 32 == 16 && *(size_t*)((uint8_t*)addr - 8) == MAGIC) {
        addr = (void*)((uint8_t*)addr - 16);
    }
    free(addr);
}

void* operator new (size_t n, const nothrow_t& tag) {
    return alloc(n);
}

void* operator new (size_t n) {
    return alloc(n);
}

void operator delete (void* addr) {
    return dealloc(addr);
}
