#include "parser.hpp"
#include <filesystem>

// ()
Expr Parser::expr1(Script& src, Local& local) {
    if (*src.p == '"' || *src.p == '\'') {
        char* str = src.p;
        do {
            if (*src.p == '\'') src.p ++;
            else if (*src.p == '\n') puts("error");
            else if (*src.p == '\r') puts("error");
            else src.p ++;
        }
        while (*str != *src.p);
        str ++; // 先頭の「"」を飛ばす 

        Bytes bytes = {0,0,0,0};
        db.emplace_back("", STRING, Bytes{})
        bytes.reg(0, std::vector<uint8_t>(str, src.p));
        
        src.p ++; // 終端の「"」を飛ばす 
        src.skip();

        return Expr{
            .opt = Expr::IMM,
            .type = STRING,
            .data = bytes
        };
    }
    else if (ID(*src.p)) {
        Script csrc = src;
        const std::string id = src.getid();

        // printf("@var %zu\n", vars->size);

        size_t i = local.vars.size() - 1;
        do if (local.vars[i].id == id) {
            return Expr{
                .opt = Expr::MODIFIABLE,
                .type = local.vars[i].type,
                .data = Bytes::emit<int32_t>(-((i+1) * 8))
            };
        }
        while (i--);

        csrc.error(1);
        printf("error: undefined variable '%s'\n", id);
        exit(-1);
    }
    else if (NUM(*src.p)) {
        int64_t n = 0;
        do n = n * 10 + (*src.p ++) - '0';
        while (NUM(*src.p));

        // double
        if (*src.p == '.' && NUM(src.p[1])) {
            src.p ++;

            double d = n;
            double u = 0.1;

            do {
                d += u * ((*src.p ++) - '0');
                u *= 0.1;
            }
            while (NUM(*src.p));
            src.skip();

            return Expr{
                .opt = Expr::IMM,
                .type = FLAOT,
                .data = d
            };
        }
        
        // long
        else {
            src.skip();
            return Expr{
                .opt = Expr::IMM,
                .type = INTEGER,
                .data = n
            };
        }
    }
    else {
        src.error(1);
        puts("error: invalid value");
        exit(-1);
    }
}

Bytes Parser::declare(Script& src, Local& local) {
    if (src.p[0] == 'l' && src.p[1] == 'e' && src.p[2] == 't' && EOI(src.p[3])) {
        src.p += 3;
        src.skip();

        // id
        if (!ID(*src.p)) {
            puts("error: not id");
            exit(-1);
        }
        auto id = src.getid();
        auto expr = expr1(src, local); // right hand value

        local.vars.emplace_back(id, expr.type);

        // 変数の最大数を更新
        if (local.maxVars < local.vars.size()) local.maxVars = local.vars.size();

        // mov [rbp - disp32], rax
        return Bytes{0x48, 0x89, 0x85} + Bytes::emit<int32_t>(-(8 * local.vars.size()));
    }
    else return statement(src, local);
}






void Parser::global(Script& src) {
    // include
    if (src.p[0] == 'i' && src.p[1] == 'n' && src.p[2] == 'c' && src.p[3] == 'l' && src.p[4] == 'u' && src.p[5] == 'd' && src.p[6] == 'e' && EOI(src.p[7])) {
        src.p += 6;
        src.skip();

        Script csrc = src;
        while (*src.p != ' ' || *src.p != '\r' || *src.p != '\n' || *src.p != '\0') *src.p ++;

        namespace fs = std::filesystem;
        std::error_code ec;
        fs::path resolved = fs::weakly_canonical(
            fs::path(src.fullpath).parent_path() / std::string(csrc.p, src.p - csrc.p),
            ec
        );
        src.skip();
        
        if (ec || !fs::exists(resolved) || !fs::is_regular_file(resolved)) {
            csrc.error(src.p - csrc.p);
            puts("error: file not found");
            exit(-1);
        }

        return parseFile(resolved.string());
    }
    
    // func
    else if (src.p[0] == 'f' && src.p[1] == 'u' && src.p[2] == 'n' && src.p[3] == 'c' && EOI(src.p[4])) {
        src.p += 4;
        src.skip();

        std::string id = src.getid();

        if (*src.p != '{') {
            src.error(1);
            puts("error: expected '{'");
            exit(-1);
        }
        src.p++;

        Local local;
        Bytes bytes;
        bytes += { // prologue
            0x55,                       // push rbp
            0x48, 0x89, 0xE5,           // mov rbp, rsp
            0x48, 0x81, 0xEC, 0,0,0,0   // sub rsp, imm32(frame_size後入れ)
        };
        while (*src.p != '}') bytes += declare(src, local);
        bytes += { // epilogue
            0x48, 0x89, 0xEC,   // mov rsp, rbp
            0x5D,               // pop rbp
            0xC3                // ret
        };

        // 変数の最大数を表示
        printf("@maxVars: %zu\n", local.maxVars);

        // フレームサイズ
        int frame_size = 32 + local.maxVars * 8;
        frame_size = (frame_size + 15) & ~15;  // 16バイト境界に丸める
        *(int32_t*)(code.main.mem + 7) = frame_size; // 埋め込み

        db.emplace_back(id, INTEGER, std::move(bytes));
    }

    else {
        src.error(1);
        puts("error: ");
        exit(-1);
    }
}

void Parser::parseFile(const std::string& fullpath) {
    if (!included_files.insert(fullpath).second) {
        // すでにinclude済み
        return;
    }

    Script src(fullpath);
    while (*src.p != '\0') global(src);
}
