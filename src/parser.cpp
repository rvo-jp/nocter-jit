#include "parser.hpp"

static void skip(Script& src) {
    while (*src.p == ' ' || *src.p == '\r' || *src.p == '\n') src.p ++;
}

static void setid(Script& src, char* mem) {
    do *mem ++ = *src.p++;
    while (ID(*src.p) || NUM(*src.p));
    *mem = '\0';
    skip(src);
}

// ()
Expr Parser::expr1(Script& src) {
    if (ID(*src.p)) {
        Script csrc = src;
        char id[64];
        setid(src, id);

        // printf("@var %zu\n", vars->size);

        size_t i = vars.size() - 1;
        do {
            if (strcmp(vars[i].id, id) == 0) {
                Expr expr(Expr::MODIFIABLE, vars[i].t, {});
                expr.u32(-((i+1) * 8));
                return expr;
            }
        }
        while (i--);

        csrc.error(1);
        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
}
