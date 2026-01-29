#pragma once
using namespace std;

class Script {
private:
    char *mem;
    const char *file;
public:
    char *p;
    void skip();
    void Script::error(int len);
    Script::Script(const char* abs_path);
    ~Script();
};
