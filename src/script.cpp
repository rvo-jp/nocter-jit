#include "parser.hpp"
#include <stdio.h>
#include <stdlib.h>

Parser::Script::Script(const std::string& fullpath) {
    this->fullpath = fullpath;

    FILE* fp = fopen(fullpath.c_str(), "rb");
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

Parser::Script::~Script() {
    free(mem);
}

void Parser::Script::error(int len) {
    
    int line = 0;
    for (char *s = p; s != mem; s --) {
        if (*s == '\n') line ++;
    }
    printf("%s, line %d:\n", fullpath, line);


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

    while (*p != '\r' && *p != '\n' && *p != '\0') putc(*p, stdout);
    putc('\n', stdout);

    while (col > 0) {
        col --;
        putc(' ', stdout);
    }
    while (len > 0) {
        len --;
        putc('^', stdout);
    }
}

void Parser::Script::skip() {
    while (*p == ' ' || *p == '\r' || *p == '\n') p ++;
}

std::string Parser::Script::getid() {
    const char *str = p;
    do *p++;
    while (ID(*p) || NUM(*p));
    std::string id(str, p - str); 
    skip();
    return id;
}