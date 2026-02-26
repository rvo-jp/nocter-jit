#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>

Parser::Script::Script(char* ptr, std::string fullpath_)
    : p(ptr), fullpath(std::move(fullpath_)) {}

void Parser::Script::error(int len, const std::string& msg, int exc) {
    
    int line = 0;
    for (char *s = p; s != mem; s --) {
        if (*s == '\n') line ++;
    }
    std::cout << fullpath << ", line " << line << ":\n";

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

    std::cout << "\n" << msg << "\n";

    if (exc != 0) exit(exc);
}

void Parser::Script::skip(int len) {
    p += len;
    while (*p == ' ' || *p == '\r' || *p == '\n') p ++;
}

std::string Parser::Script::getid() {
    const char *str = p;
    do *p++;
    while (ID(*p) || NUM(*p));
    std::string id(str, p - str); 
    skip(0);
    return id;
}