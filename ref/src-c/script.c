#include "script.h"
#include <stdio.h>
#include <stdlib.h>

script loadscript(const char* abs_path) {
    FILE* fp = fopen(abs_path, "rb");
    if (!fp) {
        puts("error: file not found");
        exit(-1);
    }

    fseek(fp, 0, SEEK_END);
    size_t flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* mem = malloc(flen + 1);
    fread(mem, 1, flen, fp);
    mem[flen] = '\0';
    fclose(fp);

    return (script){
        .file = abs_path,
        .head = mem,
        .p = mem
    };
}

void skip(script* src) {
    while (*src->p == ' ' || *src->p == '\r' || *src->p == '\n') src->p ++;
}

void error(script* src, int len) {
    
    int line = 0;
    for (char *p = src->p; p != src->head; p --) {
        if (*p == '\n') line ++;
    }
    printf("%s, line %d:\n", src->file, line);


    int col = 0;

    // 行の先頭に移動
    while (src->p != src->head && src->p[-1] != '\n') {
        src->p --;
        col ++;
    }
    // インデント分戻す
    while (*src->p == ' ') {
        src->p ++;
        col --;
    }

    while (*src->p != '\r' && *src->p != '\n') putc(*src->p, stdout);

    while (col > 0) {
        col --;
        putc(' ', stdout);
    }
    while (len > 0) {
        len --;
        putc('^', stdout);
    }
}
