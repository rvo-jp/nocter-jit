#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// rax = 式の評価結果: 演算用のレジスタ（アキュムレータ）
// rdx = 一時退避: データの一時記憶用（データレジスタ）

// 変数は rspベース disp32 2パス固定フレーム
// 最終目標は c runtime に一切頼らない構成。OS命令使う
/**
 * GCシステム
 * 1. プログラム開始時にヒープ領域を確保
 * 2. 新しいオブジェクトはヒープ末尾に積む
 * 3. 追加できなくなったら GC を実行
 * 4. GC は「使われているオブジェクト」をマークする
 * 5. マークされなかった領域は空きとして扱う
 * 6. 新しい割り当ては空き or 末尾に行う
 */
#include "bytecode.h"

typedef enum cmd {
    MOV,
    ADD
} cmd;

typedef enum type {
    ANY,
    STRING,
    INTEGER
} type;


typedef struct variable {
    char id[64];
    type type;
} variable;

typedef struct variables {
    variable* mem;
    size_t size;
    size_t max;
} variables;

// void* gc_alloc(size_t size, ObjType type) {
//     Obj* obj = malloc(size);
//     obj->type = type;
//     obj->marked = 0;
//     obj->next = gc.objects;
//     gc.objects = obj;

//     gc.bytes_allocated += size;

//     if (gc.bytes_allocated > gc.next_gc) {
//         gc_collect();
//     }

//     return obj;
// }

void skip(char** p) {
    while (**p == ' ' || **p == '\r' || **p == '\n') (*p) ++; 
}

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 
#define NUM(c) (c >= '0' && c <= '9')

void setid(char** p, char* mem) {
    do *mem ++ = *(*p)++;
    while (ID(**p) || NUM(**p));
    *mem = '\0';
    skip(p);
}


// void assign(char** p) {
//     if ((*p)[0] == '+' && (*p)[1] == '=') {
//         (*p) += 2;
//         skip(p);

        
//     }
// }

type parse_value(char** p, bytecode *code, variables* vars, cmd op, type ty) {
    if ((*p)[0] == 't' && (*p)[1] == 'r' && (*p)[2] == 'u' && (*p)[3] == 'e' && !ID((*p)[4])) {
        (*p) += 4;
        skip(p);

        uint8_t byte[] = {
            0x48, 0x83, 0xC8, 0xFF   // or rax, -1
        };
        append(code, byte, sizeof(byte));
    }
    else if ((*p)[0] == 'f' && (*p)[1] == 'a' && (*p)[2] == 'l' && (*p)[3] == 's' && (*p)[4] == 'e' && !ID((*p)[5])) {
        (*p) += 5;
        skip(p);

        uint8_t byte[] = {
            0x48, 0x31, 0xC0   // xor rax, rax
        };
        append(code, byte, sizeof(byte));
    }
    else if (ID(**p)) {
        char id[64];
        setid(p, id);

        printf("@var %zu\n", vars->size);

        for (int i = vars->size - 1; i >= 0; i --) {
            variable var = vars->mem[i];
            if (strcmp(var.id, id) == 0) {
                if (ty != ANY && var.type != ty) {
                    puts("type-error");
                    exit(-1);
                }
                // printf("%zu %s\n", i+1, var.id);

                uint8_t byte[] = {
                    0x48, 0, 0x85, 0,0,0,0   // mov rax, [rbp - disp32]
                };
                switch (op) {
                    case MOV: byte[1] = 0x8B; break;
                    case ADD: byte[1] = 0x03; break;
                }
                *(int32_t*)(byte + 3) = -((i+1) * 8);

                append(code, byte, sizeof(byte));
                return var.type;
            }
        }

        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
    else if (**p == '"' || **p == '\'') {
        if (ty != ANY && ty != STRING) {
            puts("type-error");
            exit(-1);
        }

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

        dbstring(code, str, size);
        
        // uint8_t byte[] = {
        //     0x48,0xC7,0xC1, 0,0,0,0,        // mov rcx, size+1
        //     0x48,0xB8, 0,0,0,0,0,0,0,0,     // mov rax, malloc
        //     0xFF,0xD0,                      // call rax

        //     0x48,0x89,0xC7,                 // mov rdi, rax
        //     0x48,0x8D,0x35, 0,0,0,0,        // lea rsi, [rip + str]
        //     0x48,0xC7,0xC1, 0,0,0,0,        // mov rcx, size+1
        //     0xF3,0xA4                       // rep movsb
        // };
        // *(uint64_t*)(byte + 9) = (uint64_t)malloc;
        // *(uint32_t*)(byte + 3) = (uint32_t)(size + 1);
        // *(uint32_t*)(byte + 32) = (uint32_t)(size + 1);
        // *(int32_t*)(byte + 25) = (int32_t)(-(code->dbsize + code->size + 29));

        uint8_t byte[] = {
            0x48,0x8D,0x05, 0,0,0,0     // lea rax, [rip + disp32]
        };
        *(int32_t*)(byte + 3) = (int32_t)(-(code->dbsize + code->size + 7));

        append(code, byte, sizeof(byte));

        // gc.objects = こいつ(rax)
        // [ type | marked | next(gc.objects) | len | 'A' 'A' 'A' '\0' ]

        return STRING;
    }
    else if (NUM(**p)) {
        if (ty != ANY && ty != INTEGER) {
            puts("type-error");
            exit(-1);
        }

        int64_t n = 0;
        do n = n * 10 + (*(*p) ++) - '0';
        while (NUM(**p));
        skip(p);

        uint8_t byte[] = {
            0x48, 0xB8, 0,0,0,0,0,0,0,0   // mov rax, imm64
        };
        *(int64_t*)(byte + 2) = n;
        
        // printf("@n: %ld\n", n);
        append(code, byte, sizeof(byte));

        return INTEGER;
    }
    else {
        puts("error: invalid value");
        exit(-1);
    }
}

type parse_expr(char** p, bytecode *code, variables* vars, cmd op, type ty) {
    type res = parse_value(p, code, vars, op, ty);

    for (;;)
    if (**p == '+') {
        (*p) ++;
        skip(p);
        res = parse_value(p, code, vars, ADD, res);
    }
    else return res;
}


void iputs(int64_t n) {
    printf("%ld\n", n);
}

void parse_statement(char **p, bytecode *code, variables* vars) {
    if ((*p)[0] == 'p' && (*p)[1] == 'u' && (*p)[2] == 't' && (*p)[3] == 's' && !ID((*p)[4])) {
        puts("@puts");
        (*p) += 4;
        skip(p);

        parse_expr(p, code, vars, MOV, STRING);

        uint8_t byte[] = {
            0x48,0x89,0xC1,                 // mov rcx, rax
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, imm64
            0xFF,0xD0,                      // call rax
        };
        *(uint64_t*)(byte + 5) = (uint64_t)puts;

        append(code, byte, sizeof(byte));
    }
    else if ((*p)[0] == 'i' &&  (*p)[1] == 'p' && (*p)[2] == 'u' && (*p)[3] == 't' && (*p)[4] == 's' && !ID((*p)[5])) {
        (*p) += 5;
        skip(p);

        parse_expr(p, code, vars, MOV, INTEGER);

        uint8_t byte[] = {
            0x48,0x89,0xC1,                 // mov rcx, rax
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, imm64
            0xFF,0xD0,                      // call rax
        };
        *(uint64_t*)(byte + 5) = (uint64_t)iputs;

        append(code, byte, sizeof(byte));
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

        // right hand value
        var.type = parse_expr(p, code, vars, MOV, ANY);

        // resister
        int i = vars->size;
        vars->size ++;
        vars->mem = realloc(vars->mem, vars->size * sizeof(variable));
        vars->mem[i] = var;

        // 変数の最大数を更新
        if (vars->max < vars->size) vars->max = vars->size;

        uint8_t byte[] = {
            0x48, 0x89, 0x85, 0,0,0,0   // mov [rbp - disp32], rax
        };
        *(int32_t*)(byte + 3) = -(8 * (i + 1));

        append(code, byte, sizeof(byte));

        printf("@let %zu %s\n", vars->size, vars->mem[vars->size - 1].id);
    }
    else if (**p == '{') {
        size_t size = vars->size;
        (*p) ++;
        skip(p);
        while (**p != '}') parse_statement(p, code, vars);
        (*p) ++;
        skip(p);
        vars->size = size;
        printf("@} %zu\n", vars->size);
    }
    else {
        // parse_expr(p, code, vars);
        // skip(p);
        puts("error: state");
        puts(*p);
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
        .size = 0,
        .dbmem = NULL,
        .dbsize = 0
    };

    variables vars = {
        .mem = NULL,
        .size = 0,
        .max = 0
    };


    uint8_t prologue[] = {
        0x55,                       // push rbp
        0x48, 0x89, 0xE5,           // mov rbp, rsp
        0x48, 0x81, 0xEC, 0,0,0,0   // sub rsp, imm32(frame_size)
    };
    append(&code, prologue, sizeof(prologue));


    char *source = readfile(path), *p = source;
    skip(&p);
    while (*p != '\0') parse_statement(&p, &code, &vars);
    free(vars.mem);
    free(source);

    uint8_t epilogue[] = {
        0x48, 0x89, 0xEC,   // mov rsp, rbp
        0x5D,               // pop rbp
        0xC3                // ret
    };
    append(&code, epilogue, sizeof(epilogue));

    // 変数の最大数を表示
    printf("@vars.max = %zu\n", vars.max);

    // フレームサイズ
    int frame_size = 32 + vars.max * 8;
    frame_size = (frame_size + 15) & ~15;  // 16バイト境界に丸める
    *(int32_t*)(code.mem + 7) = frame_size; // 埋め込み

    // db埋め込み
    uint8_t jmp[] = {
        0xE9, 0,0,0,0,     // jmp +N
    };
    *(int32_t*)(jmp + 1) = code.dbsize;
    dbappend(&code, jmp, sizeof(jmp));



    // 実行可能メモリを作成
    uint8_t* execode = (uint8_t*)VirtualAlloc(
        NULL, code.size + code.dbsize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!execode) return NULL;

    memcpy(execode, code.dbmem, code.dbsize);
    memcpy(execode + code.dbsize, code.mem, code.size);
    free(code.dbmem);
    free(code.mem);

    for (int i = 0; i < code.size + code.dbsize; i ++) printf("%x ", execode[i]);
    puts("");

    return (func_t)execode;
}

int main() {
    puts("@version 1.0.0");

    func_t exe = compile("./test.nct");
    puts("@compile done\n");

    exe();
    return 0;
}
