#include "script.hpp"
#include <stdio.h>
#include <stdlib.h>

Script::Script(const char* abs_path) {
    FILE* fp = fopen(abs_path, "rb");
    if (!fp) {
        puts("error: file not found");
        exit(-1);
    }

    fseek(fp, 0, SEEK_END);
    size_t flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    mem = p = (char *)malloc(flen + 1);
    fread(p, 1, flen, fp);
    p[flen] = '\0';
    fclose(fp);
}

Script::~Script() {
    free(mem);
}

void Script::error(int len) {
    
    int line = 0;
    for (char *s = p; s != mem; s --) {
        if (*s == '\n') line ++;
    }
    printf("%s, line %d:\n", file, line);


    int col = 0;

    // 行の先頭に移動
    while (p != mem && p[-1] != '\n') {
        p --;
        col ++;
    }
    // インデント分戻す
    while (*p == ' ') {
        p ++;
        col --;
    }

    while (*p != '\r' && *p != '\n') putc(*p, stdout);

    while (col > 0) {
        col --;
        putc(' ', stdout);
    }
    while (len > 0) {
        len --;
        putc('^', stdout);
    }
}
