#include "parser.hpp"
#include <filesystem>
#include <fstream>

// ()
Parser::Expr Parser::expr1(Script& src, Local& local) {
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

        size_t len = src.p - str;
        char* ptr = (char*)std::malloc(len + 1);
        std::memcpy(ptr, str, len);
        ptr[len] = '\0';

        src.p ++; // 終端の「"」を飛ばす 
        src.skip(0);

        return Expr{
            .opt = Expr::IMM,
            .type = Type{.id = "String"},
            .data = reinterpret_cast<int64_t>(ptr)
        };
    }
    else if (ID(*src.p)) {
        Script csrc = src;
        const std::string id = src.getid();

        // printf("@var %zu\n", vars->size);

        for (int64_t i = local.vars.size() - 1; i >= 0; i--) {
            if (local.vars[i].id == id) {
                return Expr{
                    .opt = Expr::VAR,
                    .type = local.vars[i].type,
                    .data = i * 8
                };
            }
        }

        csrc.error(1, "error: undefined variable '" + id + "'", -1);
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
            src.skip(0);

            return Expr{
                .opt = Expr::IMM,
                .type = Type{.id = "Float"},
                .data = d
            };
        }
        
        // long
        else {
            src.skip(0);
            return Expr{
                .opt = Expr::IMM,
                .type = Type{.id = "Int"},
                .data = n
            };
        }
    }
    else {
        src.error(1, "error: invalid value", -1);
    }
}

// + -
Parser::Expr Parser::expr2(Script& src, Local& local) {
    auto expr = expr1(src, local);

    return expr;
}

// call()
Parser::Expr Parser::expr3(Script& src, Local& local) {
    auto expr = expr2(src, local);

    return expr;
}

Parser::Bytes Parser::statement(Script& src, Local& local) {
    if (src.p[0] == 'n' && src.p[1] == 'p' && src.p[2] == 'u' && src.p[3] == 't' && src.p[4] == 's' && EOI(src.p[5])) {
        puts("@nputs");
        src.skip(5);

        Script csrc = src;
        auto expr = expr2(src, local);

        if (expr.type.id != "String") {
            csrc.error(1, "type-error: expected 'String'", -1);
        }

        Bytes{
            0x48,0x89,0xC1,                 // mov rcx, rax
            0x48,0xB8,0,0,0,0,0,0,0,0,      // mov rax, imm64
            0xFF,0xD0,                      // call rax
        }.embed<uint64_t>(5, reinterpret_cast<uint64_t>(puts));
    }
    else if (src.p[0] == 'f' && src.p[1] == 'o' && src.p[2] == 'r' && EOI(src.p[3])) {

    }
    else {
        auto expr = expr2(src, local);
    }
}

Parser::Bytes Parser::declare(Script& src, Local& local) {
    if (src.p[0] == 'l' && src.p[1] == 'e' && src.p[2] == 't' && EOI(src.p[3])) {
        src.skip(3);

        // id
        if (!ID(*src.p)) {
            src.error(1, "error: not id", -1);
        }
        auto id = src.getid();

        if (*src.p != '=') {
            src.error(1, "error: expected '='", -1);
        }
        auto expr = expr1(src, local); // right hand value

        Bytes bytes;
        if (expr.opt == Expr::RAX) bytes.append(expr.get<Bytes>());


        local.vars.emplace_back(id, expr.type);
        local.size += 8;
        // 変数の最大数を更新
        if (local.max_size < local.size) local.max_size = local.size;
        
        // mov [rsp + offset(i)], rax
        int offset = 32 + (local.vars.size() - 1) * 8;
        if (offset <= 127) {
            bytes.append(Bytes{0x48, 0x8B, 0x44, 0x24, 0}.embed<int8_t>(4, offset));
        }
        else {
            bytes.append(Bytes{0x48, 0x8B, 0x84, 0x24, 0,0,0,0}.embed<int32_t>(4, offset));
        }
        return bytes;
    }
    else return statement(src, local);
}




namespace fs = std::filesystem;

void Parser::global(Script& src) {
    // include
    if (src.p[0] == 'i' && src.p[1] == 'n' && src.p[2] == 'c' && src.p[3] == 'l' && src.p[4] == 'u' && src.p[5] == 'd' && src.p[6] == 'e' && EOI(src.p[7])) {
        src.skip(6);

        Script csrc = src;
        while (*src.p != ' ' || *src.p != '\r' || *src.p != '\n' || *src.p != '\0') *src.p ++;

        fs::path canon = fs::weakly_canonical(fs::path(src.fullpath).parent_path() / std::string(csrc.p, src.p - csrc.p));
        src.skip(0);
        
        if (!fs::exists(canon) || !fs::is_regular_file(canon)) {
            csrc.error(src.p - csrc.p, "error: file not found", -1);
        }

        include(canon.string());
    }

    // func
    else if (src.p[0] == 'f' && src.p[1] == 'u' && src.p[2] == 'n' && src.p[3] == 'c' && EOI(src.p[4])) {
        src.skip(4);

        std::string id = src.getid();
        Script code = src;

        if (*src.p != '{') src.error(1, "error: expected '{'", -1);
        int block = 0;
        do { // もっと正確に
            if (*src.p == '{') block ++;
            if (*src.p == '}') block --;
            src.p++;
        }
        while (block > 0);
        src.skip(0);

        db.emplace_back(id, Type{.id = "Function"}, code);
    }

    else {
        src.error(1, "error: ", -1);
    }
}

void Parser::include(const std::string& fullpath) {
    auto [it, inserted] = included_files.try_emplace(fullpath, nullptr);

    if (!inserted) return;//すでに存在

    std::ifstream ifs(fullpath, std::ios::binary);
    if (!ifs) {
        included_files.erase(it);
        throw std::runtime_error("file open failed: " + fullpath);
    }

    ifs.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    auto content = std::make_unique<std::string>();
    content->resize(size + 1);

    if (!ifs.read(content->data(), size)) {
        included_files.erase(it);
        throw std::runtime_error("file read failed: " + fullpath);
    }

    (*content)[size] = '\0';
    it->second = std::move(content);

    Script src(it->second->data(), fullpath);
    while (*src.p != '\0') global(src);
}


std::vector<uint8_t> Parser::parse(const std::string& path) {

    fs::path canon = fs::weakly_canonical(fs::absolute(path));

    if (!fs::exists(canon) || !fs::is_regular_file(canon)) {
        puts("error: file not found");
        exit(-1);
    }

    // パース処理
    Parser parser;
    parser.include(canon.string());

    
    Bytes bytes{
        0x48, 0x83, 0xEC, 0x28, // sub rsp, 40 (32 shadow + 8 align)
        0xE8, 0,0,0,0,          // call main (rel32 関数のアドレス)
        0x48, 0x83, 0xC4, 0x28, // add rsp, 40
        0xC3                    // ret
    };

    // dbをパース
    for (const auto& d : parser.db) {
        Local local;
        Bytes bytes;
        Script src = d.src; // コピーを作成

        if (*src.p == '{') src.skip(1);
        while (*src.p != '}') bytes.append(parser.declare(src, local));

        // フレームサイズ align16(local + saved + 32 + 8)
        int frame_size = (local.max_size + 32 + 8 + 15) & ~15;

        Bytes prologue; // sub rsp, frame_size
        Bytes epilogue; // add rsp, frame_size
        
        if (frame_size <= 127) {
            prologue = Bytes{0x48, 0x83, 0xEC, 0}.embed<int8_t>(3, frame_size);
            epilogue = Bytes{0x48, 0x83, 0xC4, 0, 0xC3}.embed<int8_t>(3, frame_size);
        }
        else {
            prologue = Bytes{0x48, 0x81, 0xEC, 0,0,0,0}.embed<int32_t>(3, frame_size);
            epilogue = Bytes{0x48, 0x81, 0xC4, 0,0,0,0, 0xC3}.embed<int32_t>(3, frame_size);
        }

        bytes.append(prologue.append(bytes).append(epilogue));
    }

    // relposを一気に解決
    for (const auto& rel : bytes.relpos) {
        bytes.rawBytes;
    }
}