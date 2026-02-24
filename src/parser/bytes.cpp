#include "parser.hpp"

Parser::Bytes::Bytes(std::initializer_list<uint8_t> init) : rawBytes(init) {}

Parser::Bytes Parser::Bytes::emit(const std::vector<uint8_t>& rawBytes) {
    Bytes bytes;
    bytes.rawBytes = rawBytes;
    return bytes;
}

Parser::Bytes& Parser::Bytes::append(const Bytes& other) {
    rawBytes.insert(rawBytes.end(), other.rawBytes.begin(), other.rawBytes.end());
    return *this;
}
