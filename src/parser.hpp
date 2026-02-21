#pragma once
#include <vector>
#include <variant>
#include <string>
#include <unordered_set>
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

    std::variant<int64_t, double, Bytes> data;

    int64_t as_int() const {
        return std::get<int64_t>(data);
    }
};





class Parser {
public:
    static Bytes parse();

private:
    class Script {
    public:
        Script(const std::string& fullpath);
        char *p;
        std::string& fullpath;

        // エラー吐く
        void error(int len);

        // 空白スキップ
        void skip();

        // 識別子取得（+移動）
        std::string getid();
    private:
        // ファイルの中身の実態（アロケート）
        char *mem;
    };

    class Local {
    public:
        // 変数
        struct Variable {
            const std::string id;
            Type type;
        };

        // 変数リスト
        std::vector<Variable> vars;

        // 変数が同時に生存する最大数
        int maxVars;
    };

    // ()
    Expr expr1(Script& src, Local& local);

    // if
    Bytes statement(Script& src, Local& local);

    // let
    Bytes declare(Script& src, Local& local);


    // 静的データ
    struct DB {
        std::string id;
        Type type;
        Bytes bytes;
    };
    // 静的データリスト
    std::vector<DB> db;

    /**
     * include func class
     * 静的データリストに保存
     */
    void global(Script& src);

    /**
     * ソースファイルをパースする
     * @param fullpath フルパス文字列
     * @return バイト列 / インクルード済みの場合空バイト列を返す
     */
    void parseFile(const std::string& fullpath);

    // インクルード済みのファイルのフルパス文字列リスト（2重インクルード防止）
    std::unordered_set<std::string> included_files;
};

