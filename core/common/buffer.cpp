/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/buffer.hpp"

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

  const uint8_t *Buffer::toBytes() const {
    return data_.data();
  }

  const std::string Buffer::toHex() const {
    return hex(data_.data(), data_.size());
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

  const uint8_t Buffer::operator[](size_t index) const {
    return data_[index];
  }

  uint8_t &Buffer::operator[](size_t index) {
    return data_[index];
  }

  expected::Result<Buffer, UnhexErrors> Buffer::fromHex(std::string_view hex) {
    return unhex(hex).match(
        [](const expected::Value<std::vector<uint8_t>> &value)
            -> expected::Result<Buffer, UnhexErrors> {
          return expected::Value{Buffer(value.value)};
        },
        [](const expected::Error<UnhexErrors> &error)
            -> expected::Result<Buffer, UnhexErrors> {
          return expected::Error{error.error};
        });
  }

  Buffer::Buffer(std::vector<uint8_t> v) : data_(std::move(v)) {}

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

  Buffer::Buffer(size_t size, uint8_t byte) : data_(size, byte) {}

  bool Buffer::operator==(const std::vector<uint8_t> &b) const noexcept {
    return data_ == b;
  }

  template <typename T>
  Buffer &Buffer::put_range(const T &begin, const T &end) {
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

  Buffer &Buffer::put_bytes(const uint8_t *begin, const uint8_t *end) {
    return putRange(begin, end);
  }

}  // namespace kagome::common
