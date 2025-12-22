#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 式の評価結果は必ずrax

#include "bytecode.h"

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 

typedef enum type {
    STRING
} type;

typedef struct value {
    type type;
    uint64_t dat;
} value;

typedef struct state{
    enum { LET, PUTS, EXIT } cmd;
    bytecode code;
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


state parse_statement(char **p, variables* vers) {
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
        res.code.size = sizeof(code);
        res.code.mem = malloc(res.code.size);
        memcpy(res.code.mem, code, res.code.size);
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
        res.code.size = sizeof(code);
        res.code.mem = malloc(res.code.size);
        memcpy(res.code.mem, code, res.code.size);
        return res;
    }
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
        char *p2 = ver.id;
        do *p2 ++ = *(*p)++;
        while (ID(**p));
        *p2 = '\0';
        skip(p);

        // right hand value
        value val = parse_value(p);
        ver.type = val.type;

        // resister
        vers->mem = realloc(vers->mem, (vers->size + 1) * sizeof(variable));
        vers->mem[vers->size ++] = ver;

        uint8_t code[] = {
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, dat
            0x48,0x89,0x45,-(vers->size * 8)// mov [rbp - (n * 8)], rax
        };
        *(uint64_t*)(code + 2)  = val.dat;

        state res;
        res.cmd = LET;
        res.code.size = sizeof(code);
        res.code.mem = malloc(res.code.size);
        memcpy(res.code.mem, code, res.code.size);
        return res;
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
    bytecode code;
    uint8_t shadow_space1[] = {0x48,0x83,0xEC,0x28};
    code.size = sizeof(shadow_space1);
    code.mem = malloc(code.size);
    memcpy(code.mem, shadow_space1, code.size);

    variables vars = {
        .mem = NULL,
        .size = 0
    };

    char *source = readfile(path);
    char *p = source;
    skip(&p);
    while (*p != '\0') {
        state res = parse_statement(&p, &vars);

        concat(&code, &res.code);

        // for (int i = 0; i < code.size; i ++) {
        //     printf("%d ", code.mem[i]);
        // }
        // puts("");
        skip(&p);
    }
    free(source);

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
