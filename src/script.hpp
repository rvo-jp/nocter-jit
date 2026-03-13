#include <string>

/**
 * Nocterスクリプトファイルオブジェクト
 */
class Script {
public:
    char *p;
    const std::string fullpath; // コピーする（実態あり）

    Script(char* ptr, std::string fullpath_);

    // エラー吐く
    void error(int len, const std::string& msg, int exc);
private:
    char *start;
};