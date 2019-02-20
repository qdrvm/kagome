/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_HPP
#define KAGOME_BUFFER_HPP

#include <string>
#include <vector>

namespace kagome::common {

  /**
   * @brief Class represents arbitrary (including empty) byte buffer.
   */
  class Buffer {
   private:


   public:
    using iterator = std::vector<uint8_t>::iterator;
    using const_iterator = std::vector<uint8_t>::const_iterator;

    /**
     * @brief
     * @param size
     * @param byte
     */
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

    template <typename T>
    Buffer &put_bytes(const T &begin, const T &end) {
      data_.insert(std::end(data_), begin, end);
      return *this;
    }

    const uint8_t *to_bytes() const;

    const std::vector<uint8_t> &to_vector() const;

    const std::string to_hex() const;
    static Buffer from_hex(const std::string &hex);

   private:
    std::vector<uint8_t> data_;
  };

}  // namespace kagome::common

#endif  // KAGOME_BUFFER_HPP
