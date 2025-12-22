#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 式の評価結果は必ずrax

#include "bytecode.h"

typedef enum type {
    STRING
} type;

typedef enum state {
    LET,
    PUTS,
    EXIT
} state;

typedef struct variable {
    char id[64];
    type type;
} variable;

typedef struct variables {
    variable* mem;
    size_t size;
} variables;


void skip(char** p) {
    while (**p == ' ' || **p == '\r' || **p == '\n') (*p) ++; 
}

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 

void setid(char **p, char *mem) {
    do *mem ++ = *(*p)++;
    while (ID(**p));
    *mem = '\0';
}

type parse_value(char** p, bytecode *code, variables* vers) {
    if (ID(**p)) {
        char id[64];
        setid(p, &id);

        for (size_t i = vers->size; i; i --) {
            variable ver = vers->mem[i];
            if (strcmp(ver.id, id) == 0) {
                uint8_t byte[] = {
                    // mov rax, [rbp - (i+1)*8]
                    0x48, 0x8B, 0x45, (uint8_t)(-((i+1) * 8))
                };
                append(code, byte, sizeof(byte));
                return ver.type;
            }
        }

        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
    else if (**p == '"' || **p == '\'') {
        char* stp = *p;
        do {
            if (**p == '\'') (*p) ++;
            else if (**p == '\n') puts("error");
            else if (**p == '\r') puts("error");
            else (*p) ++;
        }
        while (*stp != **p);
        stp ++;
        size_t size = (*p) - stp;
        (*p) ++;
        skip(p);

        char* mem = malloc(size + 1);   // プロセス終了まで保持
        memcpy(mem, stp, size);
        mem[size] = '\0';

        uint8_t byte[] = {
            0x48, 0xB8, 0,0,0,0,0,0,0,0   // mov rax, imm64
        };
        *(uint64_t*)(byte + 2) = (uint64_t)mem;

        append(code, byte, sizeof(byte));
        return STRING;
    }
    else {
        puts("error: invalid value");
        exit(-1);
    }
}


state parse_statement(char **p, bytecode *code, variables* vers) {
    if ((*p)[0] == 'p' && (*p)[1] == 'u' && (*p)[2] == 't' && (*p)[3] == 's' && !ID((*p)[4])) {
        puts("@puts");
        (*p) += 4;
        skip(p);
        if (parse_value(p, code, vers) != STRING) {
            puts("type-error: STRING");
            exit(-1);
        }

        uint8_t byte[] = {
            0x48,0x89,0xC1,                 // mov rcx, rax
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, imm64
            0xFF,0xD0,                      // call rax
        };
        *(uint64_t*)(byte + 5) = (uint64_t)puts;
        append(code, byte, sizeof(byte));

        return PUTS;
    }
    // else if ((*p)[0] == 'e' && (*p)[1] == 'x' && (*p)[2] == 'i' && (*p)[3] == 't' && !ID((*p)[4])) {
    //     puts("@exit");
    //     (*p) += 4;
    //     uint8_t code[] = {
    //         0x31,0xC9,                      // xor ecx, ecx
    //         0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, exit
    //         0xFF,0xD0                       // call rax
    //     };
    //     *(uint64_t*)(code + 4) = (uint64_t)exit;

    //     state res;
    //     res.cmd = EXIT;
    //     res.code.size = sizeof(code);
    //     res.code.mem = malloc(res.code.size);
    //     memcpy(res.code.mem, code, res.code.size);
    //     return res;
    // }
    else if ((*p)[0] == 'l' && (*p)[1] == 'e' && (*p)[2] == 't' && !ID((*p)[3])) {
        puts("@let");
        (*p) += 3;
        skip(p);
        variable ver;

        // id
        if (!ID(**p)) {
            puts("error: not id");
            exit(-1);
        }
        setid(p, &ver.id);
        skip(p);

        // right hand value
        ver.type = parse_value(p, code, vers);

        // resister
        vers->mem = realloc(vers->mem, (vers->size + 1) * sizeof(variable));
        vers->mem[vers->size ++] = ver;

        uint8_t byte[] = {
            0x48,0x89,0x45,(uint8_t)(-(vers->size * 8))    // mov [rbp - (i+1)*8], rax
        };
        append(code, byte, sizeof(byte));

        return LET;
    }
    // else if (**p == '{') {

    // }

    else {
        puts("error: state");
        exit(-1);
    }
}


typedef void (*func_t)();

// malloc
char* readfile(const char* path) {
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

    return source;
}

func_t compile(const char* path) {
    // compile
    bytecode code = {
        .mem = NULL,
        .size = 0
    };

    uint8_t prologue[] = {
        0x55,                   // push rbp
        0x48,0x89,0xE5,         // mov rbp, rsp
        0x48,0x83,0xEC,0x28,    // sub rsp, 0x28
    };
    append(&code, prologue, sizeof(prologue));

    variables vars = {
        .mem = NULL,
        .size = 0
    };

    char *source = readfile(path), *p = source;
    skip(&p);
    while (*p != '\0') {
        parse_statement(&p, &code, &vars);

        // for (int i = 0; i < code.size; i ++) {
        //     printf("%d ", code.mem[i]);
        // }
        // puts("");
        skip(&p);
    }
    free(source);

    uint8_t epilogue[] = {
        0x48,0x83,0xC4,0x28,    // add rsp, 0x28
        0x5D,                   // pop rbp
        0xC3                    // ret
    };
    append(&code, epilogue, sizeof(epilogue));




    uint8_t* vcode = (uint8_t*)VirtualAlloc(
        NULL, code.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    memcpy(vcode, code.mem, code.size);
    
    free(code.mem);
    return (func_t)vcode;
}

int main() {
    func_t exe = compile("./test.nct");
    exe();
}
