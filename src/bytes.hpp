#pragma once
#include <vector>
#include <initializer_list>
#include <cstring>
#include <type_traits>
#include "excutable.hpp"

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

        Bytes b;
        b.bytes.resize(sizeof(T));
        std::memcpy(b.bytes.data(), &value, sizeof(T));
        return b;
    }

    Bytes& operator+=(const Bytes& other);
    friend Bytes operator+(Bytes lhs, const Bytes& rhs);
    friend Bytes operator+(Bytes lhs, std::initializer_list<uint8_t> rhs);

    // 静的データ登録
    void reg(int pos, const size_t index);

    Excutable generate();

private:
    std::vector<uint8_t> bytes;

    struct Relpos {
        int pos; // このbytesの位置
        const size_t index; // 特定のBytesのDB内でのindex
    };
    std::vector<Relpos> rp;
};

