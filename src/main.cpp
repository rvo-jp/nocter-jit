#include "parser.hpp"
#include "excutable.hpp"
#include <stdio.h>

int main() {
    puts("@version 1.0.0");

    Bytes bytes = Parser::parse("./test.nct");

    puts("@compile done\n");

    Excutable exe = bytes.generate();
    exe.run();

    return 0;
}
