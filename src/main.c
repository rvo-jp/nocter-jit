#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*func_t)();

int main() {
    // 文字列を動的確保
    char* msg = (char*)malloc(14); 
    strcpy(msg, "Hello World\r\n");
    DWORD written;

    // 実行可能メモリ確保
    uint8_t* mem = (uint8_t*)malloc(4096);
    if (!mem) {
        puts("error: malloc");
        return 1;
    }

    // x64 マシン語（先ほどと同じ）
    uint8_t code[] = {
        0x48,0x83,0xEC,0x20,             // sub rsp,32
        0x48,0xC7,0xC1,0xF5,0xFF,0xFF,0xFF, // mov rcx,-11
        0xFF,0x15,0x0A,0x00,0x00,0x00,   // call [rip+0xA] GetStdHandle
        0x48,0x89,0xC1,                   // mov rcx, rax
        0x48,0xBA,0,0,0,0,0,0,0,0,       // mov rdx, <msg addr>
        0x49,0xC7,0xC0,0x0E,0x00,0x00,0x00, // mov r8, 14
        0x49,0xBA,0,0,0,0,0,0,0,0,       // mov r9, <written addr>
        0xFF,0x15,0x00,0x00,0x00,0x00,   // call [rip+0] WriteFile
        0x48,0x31,0xC9,                   // xor rcx, rcx
        0xFF,0x15,0x00,0x00,0x00,0x00    // call [rip+0] ExitProcess
    };

    // 動的文字列のアドレスを埋め込み
    uintptr_t msg_addr = (uintptr_t)msg;
    uintptr_t written_addr = (uintptr_t)&written;
    memcpy(code+16, &msg_addr, sizeof(msg_addr)); // RDX
    memcpy(code+27, &written_addr, sizeof(written_addr)); // R9

    // メモリにコピー
    memcpy(mem, code, sizeof(code));

    // 関数ポインタで実行
    func_t f = (func_t)mem;
    f();

    free(msg); // 実際には ExitProcessで終了するので不要
    return 0;
}
