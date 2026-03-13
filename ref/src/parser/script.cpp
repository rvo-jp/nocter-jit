#include "parser.hpp"
#include <iostream>

Parser::Script::Script(char* ptr, std::string fullpath_)
    : p(ptr), start(ptr), fullpath(std::move(fullpath_)) {}

void Parser::Script::error(int len, const std::string& msg, int exc) {
    
    size_t line = 0;
    for (char *s = p; s != start; s --) {
        if (*s == '\n') line ++;
    }
    std::cout << fullpath << ", line " << line << std::endl;

    size_t col = 0;

    // colをインクリメントしながら行の先頭に移動
    while (p != start && p[-1] != '\n') {
        p --;
        col ++;
    }
    // インデント分戻す
    while (*p == ' ') {
        p ++;
        col --;
    }

    const char* str = p;
    while (*p != '\r' && *p != '\n' && *p != '\0') p++;
    std::cout.write(str, p - str);
    std::cout << std::endl << std::string(col, ' ');
    std::cout << std::string(len, '^');
    std::cout << std::endl << msg << std::endl;

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