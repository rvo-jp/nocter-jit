#pragma once

class Script {
private:
    char *mem;
    const char *file;
public:
    char *p;
    void error(int len);
    Script(const char* abs_path);
    ~Script();
};
