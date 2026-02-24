#pragma once
#include <vector>
#include <variant>
#include <string>
#include <unordered_set>
#include <initializer_list>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include "excutable.hpp"

#define ID(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') 
#define NUM(c) (c >= '0' && c <= '9')
#define EOI(c) (!ID(c) && !NUM(c))

class Parser {
public:
    /**
     * ソースをコンパイルする
     * @param path ソースファイルの相対パス文字列
     * @return 実行用バイナリ
     */
    static std::vector<uint8_t> parse(const std::string& path);

private:
    Parser() = default;

    class Script {
    public:
        Script(const std::string& fullpath_);
        char *p;
        const std::string fullpath;

        // エラー吐く
        void error(int len, const std::string& msg, int exc);

        // 空白スキップ
        void skip(int len);

        // 識別子取得（+移動）
        std::string getid();
    private:
        // ファイルの中身の実態（アロケート）
        char *mem;
    };

    // 型
    struct Type {
        std::string id; // class {id} にあたる識別子
        std::vector<Type> tmpl;
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

        int size; // ローカル変数サイズ
        int max_size; // ローカル変数の最大サイズ
    };

    class Bytes {
    public:
        Bytes() = default;

        // 生バイト列用
        Bytes(std::initializer_list<uint8_t> init);

        // 数値エンコード用
        template <typename T>
        static Bytes emit(T value) {
            static_assert(
                std::is_integral_v<T> || std::is_floating_point_v<T>,
                "Bytes::emit<T> requires integral or floating-point type"
            );

            Bytes bytes;
            bytes.rawBytes.resize(sizeof(T));
            std::memcpy(bytes.rawBytes.data(), &value, sizeof(T));
            return bytes;
        }

        // vector用
        static Bytes emit(const std::vector<uint8_t>& rawBytes);

        // 数値埋め込み
        template <typename T>
        Bytes& embed(size_t pos, T value) {
            static_assert(
                std::is_integral_v<T> || std::is_floating_point_v<T>,
                "embed<T> requires integral or floating-point type"
            );

            std::memcpy(rawBytes.data() + pos, &value, sizeof(T));
            return *this;
        }

        // 連結
        Bytes& append(const Bytes& other);

        
        std::vector<uint8_t> rawBytes;

        struct Relpos {
            int pos; // このbytes上での展開予定の位置
            const size_t index; // 特定のBytesのDB内でのindex
        };
        std::vector<Relpos> relpos;
    };

    // 静的データ
    struct DB {
        std::string id;
        Type type;
        Bytes bytes;
    };
    // 静的データリスト
    std::vector<DB> db;



    struct Expr {
        enum option {
            IMM, // 即値（コンパイル時に確定してる定数）
            ADDR, // (uintptr_t)
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
        Bytes as_bytes() const {
            return std::get<Bytes>(data);
        }
    };

    // ()
    Expr expr1(Script& src, Local& local);

    // if
    Bytes statement(Script& src, Local& local);

    // let
    Bytes declare(Script& src, Local& local);


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

