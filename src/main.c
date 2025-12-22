#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 式の評価結果は必ずrax
// 変数は rspベース disp32 2パス固定フレーム

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
    size_t max;
} variables;


void skip(char** p) {
    while (**p == ' ' || **p == '\r' || **p == '\n') (*p) ++; 
}

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 

void setid(char** p, char* mem) {
    do *mem ++ = *(*p)++;
    while (ID(**p));
    *mem = '\0';
}

type parse_value(char** p, bytecode *code, variables* vars) {
    if (ID(**p)) {
        char id[64];
        setid(p, id);

        for (size_t i = vars->size - 1; i >= 0; i --) {
            variable var = vars->mem[i];
            if (strcmp(var.id, id) == 0) {
                uint8_t byte[] = {
                    // mov rax, [rsp + 0x20 + i*8]
                    0x48, 0x8B, 0x84, 0x24, 0,0,0,0
                };
                *(int32_t*)(byte + 4) = 0x20 + i * 8;
                append(code, byte, sizeof(byte));
                printf("@var %d %s\n", i+1, var.id);
                return var.type;
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


state parse_statement(char **p, bytecode *code, variables* vars) {
    if ((*p)[0] == 'p' && (*p)[1] == 'u' && (*p)[2] == 't' && (*p)[3] == 's' && !ID((*p)[4])) {
        puts("@puts");
        (*p) += 4;
        skip(p);
        if (parse_value(p, code, vars) != STRING) {
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
    else if ((*p)[0] == 'l' && (*p)[1] == 'e' && (*p)[2] == 't' && !ID((*p)[3])) {
        (*p) += 3;
        skip(p);
        variable var;

        // id
        if (!ID(**p)) {
            puts("error: not id");
            exit(-1);
        }
        setid(p, var.id);
        skip(p);

        // right hand value
        var.type = parse_value(p, code, vars);

        // resister
        int i = vars->size;
        vars->mem = realloc(vars->mem, (vars->size + 1) * sizeof(variable));
        vars->mem[vars->size ++] = var;

        // スタックサイズ更新
        if (vars->max < vars->size) vars->max = vars->size;

        uint8_t byte[] = {
            // mov [rsp + 0x20 + i*8], rax
            0x48, 0x89, 0x84, 0x24, 0,0,0,0
        };
        *(int32_t*)(byte + 4) = 0x20 + i * 8;
        append(code, byte, sizeof(byte));

        printf("@let %d %s\n", vars->size, vars->mem[vars->size - 1]);
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
        0x48, 0x81, 0xEC, 0,0,0,0   // sub rsp, stack_size
    };
    append(&code, prologue, sizeof(prologue));

    variables vars = {
        .mem = NULL,
        .size = 0,
        .max = 0
    };

    char *source = readfile(path), *p = source;
    skip(&p);
    while (*p != '\0') {
        parse_statement(&p, &code, &vars);
        skip(&p);
    }
    free(source);

    uint8_t epilogue[] = {
        0x48, 0x81, 0xC4, 0,0,0,0,  // add rsp, stack_size
        0xC3                        // ret
    };
    append(&code, epilogue, sizeof(epilogue));


    // スタックサイズ計算
    int stack_size = 0x20 + vars.max * 8;
    stack_size = (stack_size + 0xF) & ~0xF;   // 16byte 丸め
    
    // 埋め込み
    *(int32_t*)(code.mem + 3) = stack_size;
    *(int32_t*)(code.mem + code.size - 5) = stack_size;


    for (int i = 0; i < code.size; i ++) {
        printf("%x ", code.mem[i]);
    }
    puts("");

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
    puts("@compile done\n");
    exe();
    return 0;
}
