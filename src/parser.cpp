#include "parser.hpp"
#include <filesystem>

static void skip(Script& src) {
    while (*src.p == ' ' || *src.p == '\r' || *src.p == '\n') src.p ++;
}

static std::string getid(Script& src) {
    const char *str = src.p;
    do *src.p++;
    while (ID(*src.p) || NUM(*src.p));
    std::string id(str, src.p - str); 
    skip(src);
    return id;
}

// ()
Expr Parser::expr1(Script& src) {
    if (ID(*src.p)) {
        Script csrc = src;
        const std::string id = getid(src);

        // printf("@var %zu\n", vars->size);

        size_t i = vars.size() - 1;
        do if (vars[i].id == id) {
            return Expr{
                .opt = Expr::MODIFIABLE,
                .type = vars[i].type,
                .bytes = Bytes::emit<int32_t>(-((i+1) * 8))
            };
        }
        while (i--);

        csrc.error(1);
        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
}

// !
Expr Parser::expr2(Script& src) {
    if (*src.p == '!') {
        Script csrc = src;
        src.p += 1;
        skip(src);

        if (expr2(src).opt != Expr::COND) {
            csrc.error(1);
            puts("type-error: COND");
            exit(-1);
        }

        return Expr{
            .opt = Expr::COND,
            .type = BOOL
        };
    }
    else return expr1(src);
}

Bytes Parser::declare(Script& src) {
    // include
    if (src.p[0] == 'i' && src.p[1] == 'n' && src.p[2] == 'c' && src.p[3] == 'l' && src.p[4] == 'u' && src.p[5] == 'd' && src.p[6] == 'e' && EOI(src.p[7])) {
        src.p += 6;
        skip(src);

        Script csrc = src;
        while (*src.p != ' ' || *src.p != '\r' || *src.p != '\n' || *src.p != '\0') *src.p ++;

        namespace fs = std::filesystem;
        std::error_code ec;
        fs::path resolved = fs::weakly_canonical(
            fs::path(src.fullpath).parent_path() / std::string(csrc.p, src.p - csrc.p),
            ec
        );
        if (ec || !fs::exists(resolved) || !fs::is_regular_file(resolved)) {
            csrc.error(src.p - csrc.p);
            puts("error: file not found");
            exit(-1);
        }
        const std::string fullpath = resolved.string();

        if (!included_files.insert(fullpath).second) {
            // すでにinclude済み
            return Bytes{};
        }

        Script includeed_src(fullpath);
        Bytes code;
        while (includeed_src.p != '\0') code += declare(includeed_src);
        return code;
    }
    else return statement(src);
}

