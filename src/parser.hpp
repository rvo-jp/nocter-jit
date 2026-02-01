#pragma once
#include <vector>
#include <string>
#include "script.hpp"
#include "bytes.hpp"

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 
#define NUM(c) (c >= '0' && c <= '9')
#define EOI(c) (!ID(c) && !NUM(c))

enum Type {
    STRING,
    INTEGER,
    BOOL,
    FLAOT
};

struct Expr {
    enum option {
        IMM, // 即値（コンパイル時に確定してる定数）
        COND, // 真偽値
        MODIFIABLE, // 変数
        /**
         * その他の値
         * - 式の評価結果
         * - 関数呼び出し結果
         * - メモリロードが必要な値
         */
        VAL
    } opt;
    Type type;
    Bytes bytes;
};

class Parser {
private:
    struct Variable {
        const std::string id;
        Type type;
    };
    std::vector<Variable> vars;
    
    std::vector<std::string> includes;

    // ()
    Expr expr1(Script& src);

    // !
    Expr expr2(Script& src);

    // if
    Bytes statement(Script& src);

    // let include
    Bytes declare(Script& src);
};



