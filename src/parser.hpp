#pragma once
#include <vector>
#include "script.hpp"
#include "variable.hpp"
#include "bytes.hpp"
using namespace std;

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 
#define NUM(c) (c >= '0' && c <= '9')
#define EOI(c) (!ID(c) && !NUM(c))

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
    type t;
    Bytes bytes;
};

class Parser {
private:
    vector<Variable> vars;

    // ()
    Expr expr1(Script& src);
};
