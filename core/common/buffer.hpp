/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_HPP
#define KAGOME_BUFFER_HPP

#include <string>
#include <vector>

namespace kagome::common {

  class Buffer {
   private:
    template <typename T>
    Buffer &put_bytes(const T &begin, const T &end) {
      _data.insert(std::end(_data), begin, end);
      return *this;
    }

   public:
    using iterator = std::vector<uint8_t>::iterator;
    using const_iterator = std::vector<uint8_t>::const_iterator;

    explicit Buffer(size_t size, uint8_t byte);
    explicit Buffer(const std::vector<uint8_t> &v);

    Buffer() = default;
    Buffer(const Buffer &b) = default;
    Buffer(Buffer &&b) noexcept = default;
    Buffer(std::initializer_list<uint8_t> b);

    const uint8_t operator[](size_t index) const;
    uint8_t &operator[](size_t index);
    bool operator==(const Buffer &b) const noexcept;
    bool operator==(const std::vector<uint8_t> &b) const noexcept;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    size_t size() const;
    Buffer &put_uint8(uint8_t n);
    Buffer &put_uint32(uint32_t n);
    Buffer &put_uint64(uint64_t n);
    Buffer &put(const std::string &s);
    Buffer &put(const std::vector<uint8_t> &s);

    const uint8_t *to_bytes();

    const std::vector<uint8_t> &to_vector();

    const std::string to_hex();
    static Buffer from_hex(const std::string &hex);

   private:
    std::vector<uint8_t> _data;
  };

}  // namespace kagome::common

#endif  // KAGOME_BUFFER_HPP
