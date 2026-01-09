#include "bytecode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// code2を破壊し、code1に連結
void concat(bytecode* code1, bytecode* code2) {
    code1->main.mem = realloc(code1->main.mem, code1->main.size + code2->main.size);
    memcpy(code1->main.mem + code1->main.size, code2->main.mem, code2->main.size);
    code1->main.size += code2->main.size;
    free(code2->main.mem);
}

// codeの末尾に追加
void append(bytecode* code, uint8_t *mem, size_t size) {
    code->main.mem = realloc(code->main.mem, code->main.size + size);
    memcpy(code->main.mem + code->main.size, mem, size);
    code->main.size += size;
}

// codeのdbに追加
void dbappend(bytecode* code, uint8_t *dbmem, size_t size) {
    uint8_t *mem = malloc(code->db.size + size);

    memcpy(mem, dbmem, size);

    memcpy(mem + size, code->db.mem, code->db.size);
    free(code->db.mem);
    
    code->db.mem = mem;
    code->db.size += size;
}

// codeのdbにstring追加
void dbstring(bytecode* code, char *str, size_t size) {
    uint8_t *mem = malloc(code->db.size + size + 1);

    memcpy(mem, str, size);
    mem[size] = '\0';

    memcpy(mem + size + 1, code->db.mem, code->db.size);
    free(code->db.mem);

    code->db.mem = mem;
    code->db.size += size + 1;
}

void delete_code(bytecode code) {
    free(code.main.mem);
    free(code.db.mem);
}
