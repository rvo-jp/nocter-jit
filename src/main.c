#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 

typedef struct {
    enum { STRING } type;
    uint64_t dat;
} value;

typedef struct {
    enum { LET, PUTS, EXIT } cmd;
    uint8_t* code;
    size_t size;
} state;


void skip(char** p) {
    while (**p == ' ' || **p == '\r' || **p == '\n') (*p) ++; 
}

value parse_value(char** p) {
    if (**p == '"' || **p == '\'') {
        char* stp = *p;
        do { 
            if (**p == '\'') (*p) ++;
            if (**p == '\n') puts("error");
            if (**p == '\r') puts("error");
            (*p) ++;
        }
        while (*stp != **p);
        stp ++;
        size_t size = (*p) - stp;
        (*p) ++;
        skip(p);

        char* mem = malloc(size + 1); // プロセス終了まで保持
        memcpy(mem, stp, size);
        mem[size] = '\0';

        value res;
        res.type = STRING;
        res.dat  = (uint64_t)mem;
        return res;
    }
    else {
        puts("error: invalid value");
        exit(-1);
    }
}


state parse_statement(char **p) {
    if ((*p)[0] == 'p' && (*p)[1] == 'u' && (*p)[2] == 't' && (*p)[3] == 's' && !ID((*p)[4])) {
        puts("@puts");
        (*p) += 4;
        skip(p);
        value val = parse_value(p);

        uint8_t code[] = {
            0x48,0xB9,0,0,0,0,0,0,0,0,      // mov rcx, dat
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, puts
            0xFF,0xD0,                      // call rax
        };
        *(uint64_t*)(code + 2)  = val.dat;
        *(uint64_t*)(code + 12) = (uint64_t)puts;

        state res;
        res.cmd = PUTS;
        res.size = sizeof(code);
        res.code = malloc(res.size);
        memcpy(res.code, code, res.size);
        return res;
    }
    else if ((*p)[0] == 'e' && (*p)[1] == 'x' && (*p)[2] == 'i' && (*p)[3] == 't' && !ID((*p)[4])) {
        puts("@exit");
        (*p) += 4;
        uint8_t code[] = {
            0x31,0xC9,                      // xor ecx, ecx
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, exit
            0xFF,0xD0                       // call rax
        };
        *(uint64_t*)(code + 4) = (uint64_t)exit;

        state res;
        res.cmd = EXIT;
        res.size = sizeof(code);
        res.code = malloc(res.size);
        memcpy(res.code, code, res.size);
        return res;
    }
    // else if ((*p)[0] == 'l' && (*p)[1] == 'e' && (*p)[2] == 't' && !ID((*p)[3])) {
        
    // }
    // else if (**p == '{') {

    // }

    else {
        puts("error: state");
        puts(*p);
        exit(-1);
    }
}


typedef void (*func_t)();

func_t compile(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        puts("error: file not found");
        exit(-1);
    }

    fseek(fp, 0, SEEK_END);
    size_t flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* source = malloc(flen + 1);
    fread(source, 1, flen, fp);
    source[flen] = '\0';
    fclose(fp);

    // compile
    uint8_t shadow_space[] = {0x48,0x83,0xEC,0x28};
    size_t size = sizeof(shadow_space);
    uint8_t *code = malloc(size);
    memcpy(code, shadow_space, size);

    char *p = source;
    skip(&p);
    while (*p != '\0') {
        state res = parse_statement(&p);
        code = realloc(code, size + res.size);
        memcpy(code + size, res.code, res.size);
        size += res.size;
        free(res.code);
        skip(&p);
    }
    free(source);

    uint8_t* vcode = (uint8_t*)VirtualAlloc(
        NULL, size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    memcpy(vcode, code, size);
    free(code);
    return (func_t)vcode;
}

int main() {
    func_t exe = compile("./test.nct");
    exe();
}
