/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utility>

#include "buffer.hpp"
#include "hexutil.hpp"

namespace kagome::common {

  size_t Buffer::size() const {
    return data_.size();
  }

  Buffer &Buffer::put_uint32(uint32_t n) {
    data_.push_back(static_cast<unsigned char &&>((n >> 24) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 16) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 8) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n)&0xFF));

    return *this;
  }

  Buffer &Buffer::put_uint64(uint64_t n) {
    data_.push_back(static_cast<unsigned char &&>((n >> 56) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 48) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 40) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 32) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 24) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 16) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n >> 8) & 0xFF));
    data_.push_back(static_cast<unsigned char &&>((n)&0xFF));

    return *this;
  }

  const uint8_t *Buffer::to_bytes() const {
    return data_.data();
  }

  const std::string Buffer::to_hex() const {
    return hex(data_.data(), data_.size());
  }

  Buffer::Buffer(std::initializer_list<uint8_t> b) : data_(b) {}

  Buffer::iterator Buffer::begin() {
    return data_.begin();
  }

  Buffer::iterator Buffer::end() {
    return data_.end();
  }

  Buffer &Buffer::put_uint8(uint8_t n) {
    data_.push_back(n);
    return *this;
  }

  const uint8_t Buffer::operator[](size_t index) const {
    return data_[index];
  }

  uint8_t &Buffer::operator[](size_t index) {
    return data_[index];
  }

  Buffer Buffer::from_hex(const std::string &hex) {
    return Buffer(unhex(hex));
  }

  Buffer::Buffer(const std::vector<uint8_t> &v) : data_(v) {}

  const std::vector<uint8_t> &Buffer::to_vector() const {
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

}  // namespace kagome::common
