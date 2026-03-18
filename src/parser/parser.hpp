#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class parser {
    public:
    /**
     * ソースをコンパイルする
     * @param path ソースファイルの相対パス文字列
     * @return 実行用バイナリ
     */
    static std::vector<uint8_t> parse(const std::string& path);

    private:
    parser() = default;

    /**
     * 読み込んだファイルの実態
     * パーサのそれぞれの機能は、先頭からの位置をもとに、ここから参照する
     */
    std::unordered_map<std::string, std::unique_ptr<std::string>> included_files;

    /**
     * グローバル変数
     */
    class grobal_variable {
        std::string id;
        class_t type;
        std::vector<ast> tree;
    };
    /**
     * class外グローバル変数
     */
    std::vector<grobal_variable> grobal_asts;

    /**
     * AST
     * 構文ハイライトやエラーメッセージ生成も想定し、この構造がスクリプト上のどの位置で生成されたか記録しておく
     */
    class ast {
        script src;
        size_t len;
    };

    class decrare_t : ast {

    };


    /**
     * include func class などのパース
     * 静的データリストに保存
     * dbにsrcを保存する予定なので、そのsrcをパースするだけ
     */
    void parse_global(script& src);
    /**
     * ソースファイルをパースする
     * @param fullpath フルパス文字列
     */
    void include(const std::string& fullpath);
};