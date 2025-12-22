#ifndef NOCTER_BYTECODE_H
#define NOCTER_BYTECODE_H

#include <stdint.h>

typedef struct bytecode {
    uint8_t* mem;
    size_t size;
} bytecode;

// code2を破壊し、code1に連結
void concat(bytecode* code1, bytecode* code2);

#endif