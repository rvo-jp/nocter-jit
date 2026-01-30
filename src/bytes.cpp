#include "bytes.hpp"

Bytes::Bytes(std::initializer_list<uint8_t> init) : bytes(init) {}

Bytes& Bytes::operator+=(const Bytes& other) {
    bytes.insert(bytes.end(), other.bytes.begin(), other.bytes.end());
    return *this;
}

Bytes operator+(Bytes lhs, const Bytes& rhs) {
    lhs += rhs;
    return lhs;
}

Bytes operator+(Bytes lhs, std::initializer_list<uint8_t> rhs) {
    lhs += Bytes(rhs);
    return lhs;
}
