#ifndef NOCTER_BYTECODE_H
#define NOCTER_BYTECODE_H

#include <stdint.h>
#include <stdlib.h>

// 実行可能メモリ
typedef void (*func_t)();

typedef struct byte {
    uint8_t* mem;
    size_t size;
} byte;

typedef struct bytecode {
    byte main;
    byte db;
} bytecode;


// codeを解放
void delete_code(bytecode code);

// code2を破壊し、code1に連結
void concat(bytecode* code1, bytecode* code2);

// codeの末尾に追加
void append(bytecode* code, uint8_t *mem, size_t size);

// codeのdbに追加
void dbappend(bytecode* code, uint8_t *dbmem, size_t size);

// codeのdbにstring追加
void dbstring(bytecode* code, char *str, size_t size);

// codeの状態をccodeの状態に戻す
void reverse(bytecode *code, bytecode ccode);

#endif