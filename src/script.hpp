#pragma once
#include <string>

class Script {
public:
    Script::Script(const std::string& fullpath);
    char *p;
    std::string& fullpath;

    void error(int len);

private:
    char *mem;
};
