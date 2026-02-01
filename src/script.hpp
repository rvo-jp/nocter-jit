#pragma once

class Script {
public:
    Script(const char* abs_path);
    ~Script();
    char *p;

    void error(int len);

private:
    char *mem;
    const char *file;
};
