#pragma once
#include <vector>
#include <initializer_list>
#include "type.hpp"
using namespace std;

class Bytes {
public:
    Bytes() = default;

    // 生バイト列用
    Bytes(initializer_list<uint8_t> init) : bytes(init) {};

    // 数値エンコード用
    template <typename T>
    static Bytes emit(T value) {
        static_assert(
            is_integral_v<T> || is_floating_point_v<T>,
            "Bytes::emit<T> requires integral or floating-point type"
        );

        Bytes b;
        b.bytes.resize(sizeof(T));
        memcpy(b.bytes.data(), &value, sizeof(T));
        return b;
    }

private:
    vector<uint8_t> bytes;
};

