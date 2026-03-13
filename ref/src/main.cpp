#include "parser/parser.hpp"
#include "excutable.hpp"
#include <stdio.h>

int main() {
    puts("@version 1.0.0");

    Parser::parse("./test.nct");

    puts("@compile done\n");

    return 0;
}
