#include <vector>

class Excutable {
public:
    Excutable(std::vector<uint8_t> bytes);
    void run();

private:
    void *mem;
};
