#include <utility>

#include "buffer.hpp"
#include "hexutil.hpp"

namespace kagome::common {

size_t Buffer::size() const {
  return _data.size();
}

Buffer &Buffer::put_uint32(uint32_t n) {
  _data.reserve(sizeof(n));

  _data.push_back(static_cast<unsigned char &&>((n >> 24) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 16) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 8) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n) & 0xFF));

  return *this;
}

Buffer &Buffer::put_uint64(uint64_t n) {
  _data.reserve(sizeof(n));

  _data.push_back(static_cast<unsigned char &&>((n >> 56) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 48) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 40) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 32) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 24) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 16) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n >> 8) & 0xFF));
  _data.push_back(static_cast<unsigned char &&>((n) & 0xFF));

  return *this;
}

const uint8_t *Buffer::to_bytes() {
  return _data.data();
}

const std::string Buffer::to_hex() {
  return hex(_data.data(), _data.size());
}

Buffer &Buffer::put(const std::string &s) {
  return put_bytes(s.begin(), s.end());
}

Buffer &Buffer::put(const std::vector<uint8_t> &s) {
  return put_bytes(s.begin(), s.end());
}

Buffer::Buffer(std::initializer_list<uint8_t> b) : _data(b) {

}

Buffer::iterator Buffer::begin() { return _data.begin(); }

Buffer::iterator Buffer::end() { return _data.end(); }

Buffer &Buffer::put_uint8(uint8_t n) {
  _data.push_back(n);
  return *this;
}

const uint8_t Buffer::operator[](size_t index) const {
  return _data[index];
}

uint8_t &Buffer::operator[](size_t index) {
  return _data[index];
}

Buffer Buffer::from_hex(const std::string &hex) {
  return Buffer(unhex(hex));
}

Buffer::Buffer(const std::vector<uint8_t> &v) : _data(v) {

}

const std::vector<uint8_t> &Buffer::to_vector() {
  return _data;
}

bool Buffer::operator==(const Buffer &b) const noexcept {
  return _data == b._data;
}

Buffer::const_iterator Buffer::begin() const {
  return _data.begin();
}

Buffer::const_iterator Buffer::end() const {
  return _data.end();
}

Buffer::Buffer(size_t size, uint8_t byte) : _data(size, byte) {

}

bool Buffer::operator==(const std::vector<uint8_t> &b) const noexcept {
  return _data == b;
}

}
