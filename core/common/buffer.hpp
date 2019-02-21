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
   public:
    using iterator = std::vector<uint8_t>::iterator;
    using const_iterator = std::vector<uint8_t>::const_iterator;

    /**
     * @brief allocates buffer of size={@param size}, filled with {@param byte}
     */
    Buffer(size_t size, uint8_t byte);

    /**
     * @brief lvalue construct buffer from a byte vector
     */
    explicit Buffer(std::vector<uint8_t> v);

    Buffer() = default;
    Buffer(const Buffer &b) = default;
    Buffer(Buffer &&b) noexcept = default;
    Buffer(std::initializer_list<uint8_t> b);

    /**
     * @brief Accessor of byte elements given {@param index} in bytearray
     */
    const uint8_t operator[](size_t index) const;

    /**
     * @brief Accessor of byte elements given {@param index} in bytearray
     */
    uint8_t &operator[](size_t index);

    /**
     * @brief Lexicographical comparison of two buffers
     */
    bool operator==(const Buffer &b) const noexcept;

    /**
     * @brief Lexicographical comparison of buffer and vector of bytes
     */
    bool operator==(const std::vector<uint8_t> &b) const noexcept;

    /**
     * @brief Iterator, which points to begin of this buffer.
     */
    iterator begin();

    /**
     * @brief Iterator, which points to the element next to the last in this
     * buffer.
     */
    iterator end();

    /**
     * @brief Iterator, which points to begin of this buffer.
     */
    const_iterator begin() const;

    /**
     * @brief Iterator, which points to the element next to the last in this
     * buffer.
     */
    const_iterator end() const;

    /**
     * @brief Getter for size of this buffer.
     */
    size_t size() const;

    /**
     * @brief Put a 8-bit {@param n} in this buffer.
     * @return this buffer, suitable for chaining.
     */
    Buffer &put_uint8(uint8_t n);

    /**
     * @brief Put a 32-bit {@param n} number in this buffer. Will be serialized
     * as big-endian number.
     * @return this buffer, suitable for chaining.
     */
    Buffer &put_uint32(uint32_t n);

    /**
     * @brief Put a 64-bit {@param n} number in this buffer. Will be serialized
     * as big-endian number.
     * @return this buffer, suitable for chaining.
     */
    Buffer &put_uint64(uint64_t n);

    /**
     * @brief Put a string into byte buffer
     * @param s arbitrary string
     * @return this buffer, suitable for chaining.
     */
    Buffer &put(const std::string &s);

    /**
     * @brief Put a vector of bytes into byte buffer
     * @param s arbitrary vector of bytes
     * @return this buffer, suitable for chaining.
     */
    Buffer &put(const std::vector<uint8_t> &v);

    /**
     * @brief Put a array of bytes bounded by pointers into byte buffer
     * @param begin pointer to the array start
     *        end pointer to the address after the last element
     * @return this buffer, suitable for chaining.
     */
    Buffer &put_bytes(const uint8_t *begin, const uint8_t *end);

    /**
     * @brief getter for raw array of bytes
     */
    const uint8_t *to_bytes() const;

    /**
     * @brief getter for vector of vytes
     */
    const std::vector<uint8_t> &to_vector() const;

    /**
     * @brief encode bytearray as hex
     * @return hexencoded string
     */
    const std::string to_hex() const;

    /**
     * @brief Construct Buffer from hexstring
     * @param hex hexencoded string
     * @return constructed buffer
     * @throws boost::algorithm::hex_decode_error if input string is not hex
     */
    static Buffer from_hex(const std::string &hex);

   private:
    std::vector<uint8_t> data_;

    template <typename T>
    Buffer &put_range(const T &begin, const T &end);
  };

}  // namespace kagome::common

#endif  // KAGOME_BUFFER_HPP
