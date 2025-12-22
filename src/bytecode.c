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

