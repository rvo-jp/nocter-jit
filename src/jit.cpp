#include "jit.hpp"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

Jit::Jit(std::vector<uint8_t> bytes) {
#ifdef _WIN32
    mem = VirtualAlloc(
        nullptr, bytes.size(),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!mem) return;

    std::memcpy(mem, bytes.data(), bytes.size());
#else
    mem = mmap(
        nullptr, bytes.size(),
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );
    if (mem == MAP_FAILED) return;

    std::memcpy(mem, bytes.data(), bytes.size());

    __builtin___clear_cache((char*)mem, (char*)mem + bytes.size());

    mprotect(mem, bytes.size(), PROT_READ | PROT_EXEC);
#endif
}

Jit::~Jit() {
#ifdef _WIN32
    VirtualFree(mem, 0, MEM_RELEASE);
#else
    munmap(mem, 0);
#endif
}

void Jit::execute() {
    ((void (*)())mem)();
}