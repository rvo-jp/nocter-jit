#include "bytecode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// code2を破壊し、code1に連結
void concat(bytecode* code1, bytecode* code2) {
    code1->mem = realloc(code1->mem, code1->size + code2->size);
    memcpy(code1->mem + code1->size, code2->mem, code2->size);
    code1->size += code2->size;
    free(code2->mem);
}

// codeの末尾に追加
void append(bytecode* code, uint8_t *mem, size_t size) {
    code->mem = realloc(code->mem, code->size + size);
    memcpy(code->mem + code->size, mem, size);
    code->size += size;
}

// codeのdbに追加
void dbappend(bytecode* code, uint8_t *dbmem, size_t size) {
    uint8_t *mem = malloc(code->dbsize + size);

    memcpy(mem, dbmem, size);

    memcpy(mem + size, code->dbmem, code->dbsize);
    free(code->dbmem);
    
    code->dbmem = mem;
    code->dbsize += size;
}

// codeのdbにstring追加
void dbstring(bytecode* code, char *str, size_t size) {
    uint8_t *mem = malloc(code->dbsize + size + 1);

    memcpy(mem, str, size);
    mem[size] = '\0';

    memcpy(mem + size + 1, code->dbmem, code->dbsize);
    free(code->dbmem);

    code->dbmem = mem;
    code->dbsize += size + 1;
}

