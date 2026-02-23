#include "parser.hpp"

Parser::Bytes::Bytes(std::initializer_list<uint8_t> init) : bytes(init) {}

Parser::Bytes Parser::Bytes::emit(const std::vector<uint8_t>& vec) {
    Bytes b;
    b.bytes = vec;
    return b;
}

Parser::Bytes& Parser::Bytes::append(const Bytes& other) {
    bytes.insert(bytes.end(), other.bytes.begin(), other.bytes.end());
    return *this;
}
