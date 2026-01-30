#include <vector>

class Jit {
public:
    Jit(std::vector<uint8_t> bytes);
    ~Jit();
    void execute();

private:
    void *mem;
};
