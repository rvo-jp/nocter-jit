#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

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
                #ifdef _WIN32
                uint8_t byte[] = {
                    0x48, 0x8B, 0x84, 0x24, 0,0,0,0 // mov rax, [rsp + 0x20 + i*8] 
                };
                *(int32_t*)(byte + 4) = 0x20 + i * 8;

                #else
                uint8_t byte[] = {
                    0xE0, 0x03, 0x40, 0xF9   // ldr x0, [sp, #imm12]
                };

                int offset = 0x20 + i * 8;
                int imm12  = offset >> 3;

                *(uint32_t*)byte |= ((imm12 & 0xFFF) << 10);
                #endif

                append(code, byte, sizeof(byte));
                printf("@var %d %s\n", i+1, var.id);
                return var.type;
            }
        }

        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
    else if (**p == '"' || **p == '\'') {
        char* str = *p;
        do {
            if (**p == '\'') (*p) ++;
            else if (**p == '\n') puts("error");
            else if (**p == '\r') puts("error");
            else (*p) ++;
        }
        while (*str != **p);
        str ++;
        size_t size = (*p) - str;
        (*p) ++;
        skip(p);

        char* mem = malloc(size + 1);   // プロセス終了まで保持
        memcpy(mem, str, size);
        mem[size] = '\0';

        #ifdef _WIN32
        uint8_t byte[] = {
            0x48, 0xB8, 0,0,0,0,0,0,0,0   // mov rax, imm64
        };
        *(uint64_t*)(byte + 2) = (uint64_t)mem;

        #else
        uint8_t byte[] = {
            0x00, 0x00, 0x80, 0xD2,     // movz x0, #imm16
            0x00, 0x00, 0xA0, 0xF2,     // movk x0, #imm16, lsl #16
            0x00, 0x00, 0xC0, 0xF2,     // movk x0, #imm16, lsl #32
            0x00, 0x00, 0xE0, 0xF2      // movk x0, #imm16, lsl #48
        };
        uint64_t addr = (uint64_t)mem;
        uint32_t* b = (uint32_t*)byte;
        b[0] |= ((addr & 0xFFFF) << 5);         // movz x0, #imm16
        b[1] |= (((addr >> 16) & 0xFFFF) << 5); // movk x0, #imm16, lsl #16
        b[2] |= (((addr >> 32) & 0xFFFF) << 5); // movk x0, #imm16, lsl #32
        b[3] |= (((addr >> 48) & 0xFFFF) << 5); // movk x0, #imm16, lsl #48
        #endif

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

        #ifdef _WIN32
        uint8_t byte[] = {
            0x48,0x89,0xC1,                 // mov rcx, rax
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, imm64
            0xFF,0xD0,                      // call rax
        };
        *(uint64_t*)(byte + 5) = (uint64_t)puts;

        #else
        uint8_t byte[] = {
            0x10, 0x00, 0x80, 0xD2,         // movz x16, #imm16
            0x10, 0x00, 0xA0, 0xF2,         // movk x16, #imm16, lsl #16
            0x10, 0x00, 0xC0, 0xF2,         // movk x16, #imm16, lsl #32
            0x10, 0x00, 0xE0, 0xF2,         // movk x16, #imm16, lsl #48
            0x00, 0x02, 0x3F, 0xD6          // blr x16
        };
        uint64_t addr = (uint64_t)puts;
        uint32_t* b = (uint32_t*)byte;
        b[0] |= ((addr & 0xFFFF) << 5); // movz
        b[1] |= (((addr >> 16) & 0xFFFF) << 5); // movk lsl 16
        b[2] |= (((addr >> 32) & 0xFFFF) << 5); // movk lsl 32
        b[3] |= (((addr >> 48) & 0xFFFF) << 5); // movk lsl 48
        #endif

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

        #ifdef _WIN32
        uint8_t byte[] = {
            0x48, 0x89, 0x84, 0x24, 0,0,0,0 // mov [rsp + 0x20 + i*8], rax
        };
        *(int32_t*)(byte + 4) = 0x20 + i * 8;
        
        #else
        uint8_t byte[] = {
            0xE0, 0x03, 0x00, 0xF9   // str x0, [sp, #imm12]
        };

        int offset = 0x20 + i * 8;      // byte単位
        int imm12  = offset >> 3;       // ARM64 は 8byte単位

        *(uint32_t*)byte |= ((imm12 & 0xFFF) << 10);
        #endif

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


// 実行可能メモリ
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
        #ifdef _WIN32
        0x48, 0x81, 0xEC, 0,0,0,0   // sub rsp, stack_size
        #else
        0xFD, 0x7B, 0xBF, 0xA9,     // stp x29, x30, [sp, #-16]!
        0xFD, 0x03, 0x00, 0x91,     // mov x29, sp
        0xFF, 0x03, 0x00, 0xD1      // sub sp, sp, #imm12(stack_size)
        #endif
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
        #ifdef _WIN32
        0x48, 0x81, 0xC4, 0,0,0,0,  // add rsp, stack_size
        0xC3                        // ret
        #else
        0xFF, 0x03, 0x00, 0x91,     // add sp, sp, #imm12
        0xFD, 0x7B, 0xC1, 0xA8,     // ldp x29, x30, [sp], #16
        0xC0, 0x03, 0x5F, 0xD6      // ret
        #endif
    };
    append(&code, epilogue, sizeof(epilogue));

    // スタックサイズ計算
    printf("@vars.max = %d\n", vars.max);
    int stack_size = 0x20 + vars.max * 8;
    stack_size = (stack_size + 0xF) & ~0xF;   // 16byte 丸め
    
    // 埋め込み
    #ifdef _WIN32
    *(int32_t*)(code.mem + 3) = stack_size;
    *(int32_t*)(code.mem + code.size - 5) = stack_size;

    #else
    uint32_t* sub = (uint32_t*)(code.mem + 8);
    *sub |= ((stack_size & 0xFFF) << 10);

    uint32_t* add = (uint32_t*)(code.mem + code.size - 12);
    *add |= ((stack_size & 0xFFF) << 10);
    #endif

    for (int i = 0; i < code.size; i ++) printf("0x%x, ", code.mem[i]);
    puts("");

    // 実行可能メモリに移動
    #ifdef _WIN32
    uint8_t* execode = (uint8_t*)VirtualAlloc(
        NULL, code.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!execode) return NULL;

    memcpy(execode, code.mem, code.size);

    #else
    // ページサイズ取得
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t alloc_size = (code.size + pagesize - 1) & ~(pagesize - 1);

    // まず RW で確保
    uint8_t* execode = mmap(
        NULL,
        alloc_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, 0
    );
    if (execode == MAP_FAILED) return NULL;

    memcpy(execode, code.mem, code.size);

    // RX に変更（W^X）
    if (mprotect(execode, alloc_size, PROT_READ | PROT_EXEC) != 0) {
        munmap(execode, alloc_size);
        return NULL;
    }

    __builtin___clear_cache(execode, execode + code.size);
    #endif

    free(code.mem);
    return (func_t)execode;
}

int main() {
    func_t exe = compile("./test.nct");
    puts("@compile done\n");
    exe();
    return 0;
}
