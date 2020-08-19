/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/buffer.hpp"
#include "buffer.hpp"
#include "common/hexutil.hpp"

namespace kagome::common {

  size_t Buffer::size() const {
    return data_.size();
  }

  Buffer &Buffer::putUint32(uint32_t n) {
    data_.push_back(static_cast<unsigned char &&>((n >> 24) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 16) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 8) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n)&0xFF));

    return *this;
  }

  Buffer &Buffer::putUint64(uint64_t n) {
    data_.push_back(static_cast<unsigned char &&>((n >> 56u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 48u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 40u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 32u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 24u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 16u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 8u) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n)&0xFF));

    return *this;
  }

  std::string Buffer::toHex() const {
    return hex_lower(data_);
  }

  const std::string_view Buffer::toString() const {
    return std::string_view(reinterpret_cast<const char*>(data_.data()), data_.size()); // NOLINT
  }

  bool Buffer::empty() const {
    return data_.empty();
  }

  Buffer::Buffer(std::initializer_list<uint8_t> b) : data_(b) {}

  Buffer::iterator Buffer::begin() {
    return data_.begin();
  }

  Buffer::iterator Buffer::end() {
    return data_.end();
  }

  Buffer &Buffer::putUint8(uint8_t n) {
    data_.push_back(n);
    return *this;
  }

  uint8_t Buffer::operator[](size_t index) const {
    return data_[index];
  }

  uint8_t &Buffer::operator[](size_t index) {
    return data_[index];
  }

  outcome::result<Buffer> Buffer::fromHex(std::string_view hex) {
    OUTCOME_TRY(bytes, unhex(hex));
    return Buffer{std::move(bytes)};
  }

  Buffer::Buffer(std::vector<uint8_t> v) : data_(std::move(v)) {}
  Buffer::Buffer(gsl::span<const uint8_t> s) : data_(s.begin(), s.end()) {}

  const std::vector<uint8_t> &Buffer::toVector() const {
    return data_;
  }

  bool Buffer::operator==(const Buffer &b) const noexcept {
    return data_ == b.data_;
  }

  Buffer::const_iterator Buffer::begin() const {
    return data_.begin();
  }

  Buffer::const_iterator Buffer::end() const {
    return data_.end();
  }

  const uint8_t *Buffer::data() const {
    return data_.data();
  }

  uint8_t *Buffer::data() {
    return data_.data();
  }

  Buffer::Buffer(size_t size, uint8_t byte) : data_(size, byte) {}

  bool Buffer::operator==(const std::vector<uint8_t> &b) const noexcept {
    return data_ == b;
  }

  bool Buffer::operator==(gsl::span<const uint8_t> s) const noexcept {
    return std::equal(data_.begin(), data_.end(), s.begin(), s.end());
  }

  bool Buffer::operator<(const Buffer &b) const noexcept {
    return std::lexicographical_compare(begin(), end(), b.begin(), b.end());
  }

  template <typename T>
  Buffer &Buffer::putRange(const T &begin, const T &end) {
    static_assert(sizeof(*begin) == 1);
    data_.insert(std::end(data_), begin, end);
    return *this;
  }

  Buffer &Buffer::put(std::string_view s) {
    return putRange(s.begin(), s.end());
  }

  Buffer &Buffer::put(const std::vector<uint8_t> &v) {
    return putRange(v.begin(), v.end());
  }

  Buffer &Buffer::put(gsl::span<const uint8_t> s) {
    return putRange(s.begin(), s.end());
  }

  Buffer &Buffer::putBytes(const uint8_t *begin, const uint8_t *end) {
    return putRange(begin, end);
  }

  Buffer &Buffer::putBuffer(const Buffer &buf) {
    return put(buf.toVector());
  }

  void Buffer::clear() {
    data_.clear();
  }

  Buffer::Buffer(const uint8_t *begin, const uint8_t *end)
      : data_{begin, end} {}

  std::vector<uint8_t> &Buffer::toVector() {
    return data_;
  }

  Buffer &Buffer::reserve(size_t size) {
    data_.reserve(size);
    return *this;
  }

  Buffer &Buffer::resize(size_t size) {
    data_.resize(size);
    return *this;
  }

  Buffer Buffer::subbuffer(size_t offset, size_t length) const {
    return Buffer(gsl::make_span(*this).subspan(offset, length));
  }

  Buffer &Buffer::operator+=(const Buffer &other) noexcept {
    return this->putBuffer(other);
  }

  std::ostream &operator<<(std::ostream &os, const Buffer &buffer) {
    return os << buffer.toHex();
  }

}  // namespace kagome::common
