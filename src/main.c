#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

// rax = 式の評価結果
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
#include "script.h"


typedef enum type {
    STRING,
    INTEGER,
    BOOL
} type;

typedef struct expr {
    enum op {
        VALUE,
        CONDITON,
        MODIFIABLE
    } op;
    type tp;
} expr;


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

// == 0F 84
// != 0F 85
// < 0F 8C
// <= 0F 8E
// > 0F 8F
// >= 0F 8D



#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 
#define NUM(c) (c >= '0' && c <= '9')
#define EOI(c) (!ID(c) && !NUM(c))

void setid(script* src, char* mem) {
    do *mem ++ = *src->p++;
    while (ID(*src->p) || NUM(*src->p));
    *mem = '\0';
    skip(src);
}

expr parse_expr(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne);

type parse_value(script* src, bytecode *code, variables* vars) {
    script csrc = *src;
    bytecode ccode = *code;
    expr res = parse_expr(src, code, vars, 0, 0, 0);

    if (res.op == CONDITON) {
        int32_t L_eq = code->main.size;
        int32_t L_ne = code->main.size + 4;

        
    }
    else if (res.op == MODIFIABLE) {

    }
    else return res.tp;
}

// ()
expr parse_expr1(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne, bool assign) {
    if (*src->p == '(') {
        src->p += 1;
        skip(src);

        expr res = parse_expr(src, code, vars, sign, L_eq, L_ne);

        if (*src->p != ')') {
            error(src, 1);
            puts("syntax error: )");
            exit(-1);
        }
        src->p += 1;
        skip(src);

        return res;
    }
    else if (*src->p == '"' || *src->p == '\'') {
        char* str = src->p;
        do {
            if (*src->p == '\'') src->p ++;
            else if (*src->p == '\n') puts("error");
            else if (*src->p == '\r') puts("error");
            else src->p ++;
        }
        while (*str != *src->p);
        str ++;
        size_t size = src->p - str;
        src->p ++;
        skip(src);

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
        // *(int32_t*)(byte + 25) = (int32_t)(-(code->db.size + code->main.size + 29));

        uint8_t byte[] = {
            0x48,0x8D,0x05, 0,0,0,0     // lea rax, [rip + disp32]
        };
        *(int32_t*)(byte + 3) = (int32_t)(-(code->db.size + code->main.size + 7));

        append(code, byte, sizeof(byte));

        // gc.objects = こいつ(rax)
        // [ type | marked | next(gc.objects) | len | 'A' 'A' 'A' '\0' ]

        return (expr){
            .op = VALUE,
            .tp = STRING
        };
    }
    else if (src->p[0] == 't' && src->p[1] == 'r' && src->p[2] == 'u' && src->p[3] == 'e' && EOI(src->p[4])) {
        src->p += 4;
        skip(src);

        uint8_t byte[] = {
            0x48, 0x83, 0xC8, 0xFF   // or rax, -1
        };
        append(code, byte, sizeof(byte));
    }
    else if (src->p[0] == 'f' && src->p[1] == 'a' && src->p[2] == 'l' && src->p[3] == 's' && src->p[4] == 'e' && EOI(src->p[5])) {
        src->p += 5;
        skip(src);

        uint8_t byte[] = {
            0x48, 0x31, 0xC0   // xor rax, rax
        };
        append(code, byte, sizeof(byte));
    }
    else if (ID(*src->p)) {
        script csrc = *src;
        char id[64];
        setid(src, id);

        // printf("@var %zu\n", vars->size);

        for (int i = vars->size - 1; i >= 0; i --) {
            variable var = vars->mem[i];
            if (strcmp(var.id, id) == 0) {
                // printf("%zu %s\n", i+1, var.id);

                if (assign) {
                    uint8_t byte[] = {
                        0x48, 0x89, 0x85, 0,0,0,0   // mov [rbp - disp32], rax
                    };
                    *(int32_t*)(byte + 3) = -((i+1) * 8);
                    append(code, byte, sizeof(byte));
                }
                else {
                    uint8_t byte[] = {
                        0x48, 0x8B, 0x85, 0,0,0,0   // mov rax, [rbp - disp32]
                    };
                    *(int32_t*)(byte + 3) = -((i+1) * 8);
                    append(code, byte, sizeof(byte));
                }

                return (expr){
                    .op = MODIFIABLE,
                    .tp = var.type
                };
            }
        }

        error(&csrc, 1);
        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
    else if (NUM(*src->p)) {
        int64_t n = 0;
        do n = n * 10 + (*src->p ++) - '0';
        while (NUM(*src->p));
        skip(src);

        uint8_t byte[] = {
            0x48, 0xB8, 0,0,0,0,0,0,0,0   // mov rax, imm64
        };
        *(int64_t*)(byte + 2) = n;

        // printf("@n: %ld\n", n);
        append(code, byte, sizeof(byte));

        return (expr){
            .op = VALUE,
            .tp = INTEGER
        };
    }
    else {
        error(src, 1);
        puts("error: invalid value");
        exit(-1);
    }
}

// !
expr parse_expr2(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne, bool assign) {
    if (*src->p == '!') {
        src->p += 1;
        skip(src);

        if (parse_expr2(src, code, vars, sign, L_ne, L_eq, assign).op != CONDITON) {
            error(src, 1);
            puts("type-error: COND");
            exit(-1);
        }

        return (expr){
            .op = CONDITON,
            .tp = BOOL
        };
    }
    
    return parse_expr1(src, code, vars, sign, L_eq, L_ne, assign);
}

// *
expr parse_expr3(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne, bool assign) {
    expr res = parse_expr2(src, code, vars, sign, L_eq, L_ne, assign);

    for (;;)
    if (*src->p == '*') {
        if (res.tp != INTEGER) {
            puts("type-error: INT");
            exit(-1);
        }

        uint8_t byte1[] = {
            0x50    // push rax
        };
        append(code, byte1, sizeof(byte1));

        src->p ++;
        skip(src);
        if (parse_expr2(src, code, vars, sign, L_eq, L_ne, assign).tp != INTEGER) {
            puts("type-error: INT");
            exit(-1);
        }

        uint8_t byte2[] = {
            0x59,                   // pop rcx
            0x48, 0x0F, 0xAF, 0xC1  // imul rax, rcx
        };
        append(code, byte2, sizeof(byte2));
    }
    else return res;
}

// +
expr parse_expr4(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne, bool assign) {
    expr res = parse_expr3(src, code, vars, sign, L_eq, L_ne, assign);

    for (;;)
    if (*src->p == '+') {
        if (res.tp != INTEGER) {
            puts("type-error: INT");
            exit(-1);
        }

        uint8_t byte1[] = {
            0x50    // push rax
        };
        append(code, byte1, sizeof(byte1));

        src->p ++;
        skip(src);
        if (parse_expr3(src, code, vars, sign, L_eq, L_ne, assign).tp != INTEGER) {
            puts("type-error: INT");
            exit(-1);
        }

        uint8_t byte2[] = {
            0x59,               // pop rcx
            0x48, 0x01, 0xC8    // add rax, rcx
        };
        append(code, byte2, sizeof(byte2));
    }
    else return res;
}

// == !=
expr parse_expr5(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne, bool assign) {
    expr res = parse_expr4(src, code, vars, sign, L_eq, L_ne, assign);

    for (;;)
    if (src->p[0] == '=' && src->p[1] == '=') {
        uint8_t byte1[] = {
            0x50    // push rax
        };
        append(code, byte1, sizeof(byte1));

        src->p += 2;
        skip(src);
        if (parse_expr4(src, code, vars, sign, L_eq, L_ne, assign).tp != res.tp) {
            puts("type-error: ==");
            exit(-1);
        }

        uint8_t byte2[] = {
            0x59,                   // pop rcx
            0x48, 0x39, 0xC8,       // cmp rax, rcx
            0x0F, 0x00, 0,0,0,0     // je rel32
        };
        *(uint8_t*)(byte2 + 5) = sign ? 0x84 : 0x85;
        *(int32_t*)(byte2 + 6) = L_eq - (code->main.size + sizeof(byte2));

        append(code, byte2, sizeof(byte2));
        res = (expr){
            .op = CONDITON,
            .tp = BOOL
        };
    }
    else if (src->p[0] == '!' && src->p[1] == '=') {
        uint8_t byte1[] = {
            0x50    // push rax
        };
        append(code, byte1, sizeof(byte1));

        src->p += 2;
        skip(src);
        if (parse_expr4(src, code, vars, sign, L_eq, L_ne, assign).tp != res.tp) {
            puts("type-error: !=");
            exit(-1);
        }

        uint8_t byte2[] = {
            0x59,                   // pop rcx
            0x48, 0x39, 0xC8,       // cmp rax, rcx
            0x0F, 0x00, 0,0,0,0     // jne rel32
        };
        *(uint8_t*)(byte2 + 5) = sign ? 0x85 : 0x84;
        *(int32_t*)(byte2 + 6) = L_eq - (code->main.size + sizeof(byte2));

        append(code, byte2, sizeof(byte2));
        res = (expr){
            .op = CONDITON,
            .tp = BOOL
        };
    }
    else return res;
}

// &&
expr parse_expr6(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne, bool assign) {
    script csrc = *src;
    bytecode ccode = *code;
    expr res = parse_expr5(src, code, vars, sign, L_eq, L_ne, assign);

    if (src->p[0] == '&' && src->p[1] == '&') {
        if (res.op != CONDITON) {
            error(src, 2);
            puts("type-error: _ &&");
            exit(-1);
        }
        *src = csrc;
        reverse(code, ccode);
        parse_expr5(src, code, vars, sign, L_eq, L_ne, assign);

        do {
            src->p += 2;
            skip(src);
            if (parse_expr5(src, code, vars, sign, L_eq, L_ne, assign).op != CONDITON) {
                puts("type-error: && _");
                exit(-1);
            }
        }
        while (src->p[0] == '&' && src->p[1] == '&');
    }
    
    return res;
}

// =
expr parse_expr(script* src, bytecode *code, variables* vars, bool sign, int32_t L_eq, int32_t L_ne) {
    script csrc = *src;
    bytecode ccode = *code;
    expr res = parse_expr6(src, code, vars, sign, L_eq, L_ne, 0);

    if (*src->p == '=') {
        if (res.op != MODIFIABLE) {
            error(src, 1);
            puts("error: expected modifiable lvalue");
            exit(-1);
        }
        script rsrc = *src;

        reverse(code, ccode);
        src->p += 1;
        skip(src);
        if (parse_expr(src, code, vars, sign, L_eq, L_ne).tp != res.tp) {
            error(&rsrc, 1);
            puts("type-error: ");
            exit(-1);
        }
        parse_expr6(&csrc, code, vars, sign, L_eq, L_ne, 1);

        return res;
    }

    return res;
}



void iputs(int64_t n) {
    printf("%ld\n", n);
}

void parse_statement_single(script* src, bytecode *code, variables* vars);

void parse_statement(script* src, bytecode *code, variables* vars) {
    if (src->p[0] == 'p' && src->p[1] == 'u' && src->p[2] == 't' && src->p[3] == 's' && EOI(src->p[4])) {
        puts("@puts");
        src->p += 4;
        skip(src);

        if (parse_expr(src, code, vars, 0, 0, 0).tp != STRING) {
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
    }
    else if (src->p[0] == 'i' &&  src->p[1] == 'p' && src->p[2] == 'u' && src->p[3] == 't' && src->p[4] == 's' && EOI(src->p[5])) {
        src->p += 5;
        skip(src);

        if (parse_expr(src, code, vars, 0, 0, 0).tp != INTEGER) {
            puts("type-error: INT");
            exit(-1);
        }

        uint8_t byte[] = {
            0x48,0x89,0xC1,                 // mov rcx, rax
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, imm64
            0xFF,0xD0,                      // call rax
        };
        *(uint64_t*)(byte + 5) = (uint64_t)iputs;

        append(code, byte, sizeof(byte));
    }
    else if (src->p[0] == 'i' &&  src->p[1] == 'f' && EOI(src->p[2])) {
        src->p += 2;
        skip(src);

        int32_t L_eq = 0, L_ne = 0, L_end = 0;
        char *cp = src->p;
        bytecode ccode = *code;

        if (parse_expr(src, code, vars, true, 0, 0).op != CONDITON) {
            puts("type-error: CONDITION");
            exit(-1);
        }
        L_eq = code->main.size;
        parse_statement(src, code, vars);

        if (src->p[0] == 'e' && src->p[1] == 'l' && src->p[2] == 's' && src->p[3] == 'e' && EOI(src->p[4])) {
            uint8_t byte[] = {
                0xE9, 0,0,0,0,  // jmp
            };
            append(code, byte, sizeof(byte));

            L_ne = code->main.size;
            src->p += 4;
            skip(src);
            parse_statement(src, code, vars);
            L_end = code->main.size;
        }
        else L_ne = code->main.size;

        src->p = cp;
        reverse(code, ccode);

        parse_expr(src, code, vars, false, L_ne, L_eq);
        parse_statement(src, code, vars);

        if (L_end != 0) {
            uint8_t byte[] = {
                0xE9, 0,0,0,0,  // jmp
            };
            *(int32_t*)(byte + 1) = L_end - (code->main.size + sizeof(byte));
            append(code, byte, sizeof(byte));

            src->p += 4;
            skip(src);
            parse_statement(src, code, vars);
        }
    }
    else if (*src->p == '{') {
        size_t size = vars->size;
        src->p ++;
        skip(src);
        while (*src->p != '}') parse_statement_single(src, code, vars);
        src->p ++;
        skip(src);
        vars->size = size;
        printf("@} %zu\n", vars->size);
    }
    else {
        char *cp = src->p;
        bytecode ccode = *code;
        
        expr res = parse_expr(src, code, vars, 0, 0, 0);

        if (res.op == CONDITON) {
            int32_t L_end = code->main.size;
            src->p = cp;
            reverse(code, ccode);
            parse_expr(src, code, vars, 0, L_end, L_end);
        }
    }
}

void parse_statement_single(script* src, bytecode *code, variables* vars) {
    if (src->p[0] == 'l' && src->p[1] == 'e' && src->p[2] == 't' && EOI(src->p[3])) {
        src->p += 3;
        skip(src);

        // id
        if (!ID(*src->p)) {
            puts("error: not id");
            exit(-1);
        }
        variable var;
        setid(src->p, var.id);
        var.type = parse_expr(src, code, vars, 0, 0, 0).tp; // right hand value

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
    else if (src->p[0] == 'i' && src->p[1] == 'm' && src->p[2] == 'p' && src->p[3] == 'o' && src->p[4] == 'r' && src->p[5] == 't' && EOI(src->p[6])) {
        src->p += 6;
        skip(src);

        char path[1024];
        size_t len = strrchr(src->file, '\\') + 1 - src->file;
        memcpy(path, src->file, len);
        char* p = path + len;

        script csrc = *src;
        while (*src->p != ' ') *p ++ = *src->p ++;
        *p = '\0';

        char abs_path[1024];
        if (GetFullPathNameA(path, 1024, abs_path, NULL) == 0) {
            error(&csrc, src->p - csrc.p);
            puts("error: file not found");
            exit(-1);
        }

        script new_src = loadscript(abs_path);
        skip(&new_src);
        while (*new_src.p != '\0') parse_statement_single(&new_src, code, vars);
        free(new_src.head);
        skip(src);
    }
    else parse_statement(src, code, vars);
}

// 実行可能メモリ
typedef void (*func_t)();



func_t compile(const char* path) {
    // compile
    bytecode code = {0};
    variables vars = {0};

    uint8_t prologue[] = {
        0x55,                       // push rbp
        0x48, 0x89, 0xE5,           // mov rbp, rsp
        0x48, 0x81, 0xEC, 0,0,0,0   // sub rsp, imm32(frame_size)
    };
    append(&code, prologue, sizeof(prologue));

    char abs_path[1024];
    if (GetFullPathNameA(path, 1024, abs_path, NULL) == 0) {
        puts("error: file not found");
        exit(-1);
    }

    script src = loadscript(abs_path);
    skip(&src);
    while (*src.p != '\0') parse_statement_single(&src, &code, &vars);
    free(src.head);

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
    *(int32_t*)(code.main.mem + 7) = frame_size; // 埋め込み
    free(vars.mem);

    // db埋め込み
    uint8_t jmp[] = {
        0xE9, 0,0,0,0,     // jmp +N
    };
    *(int32_t*)(jmp + 1) = code.db.size;
    dbappend(&code, jmp, sizeof(jmp));



    // 実行可能メモリを作成
    uint8_t* execode = (uint8_t*)VirtualAlloc(
        NULL, code.main.size + code.db.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!execode) return NULL;

    memcpy(execode, code.db.mem, code.db.size);
    memcpy(execode + code.db.size, code.main.mem, code.main.size);
    delete_code(code);

    for (int i = 0; i < code.main.size + code.db.size; i ++) printf("%x ", execode[i]);
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
