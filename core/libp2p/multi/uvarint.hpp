/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VARINT_HPP
#define KAGOME_VARINT_HPP

#include <cstdint>
#include <gsl/span>
#include <vector>

namespace libp2p::multi {

  /**
   * A C++ wrapper for varint encoding implementation, which can be found in
   * c-utils library. Encodes unsigned integers into variable-length byte arrays,
   * efficient, having both an ability to store large numbers and not wasting
   * space on small ones. Mind that the maximum length of a varint is 8 bytes
   * and it can store only unsigned integers
   * @see https://github.com/multiformats/unsigned-varint
   */
  class UVarint {
   public:
    /**
     * Constructs a varint from an unsigned integer 'number'
     * @param number
     */
    explicit UVarint(uint64_t number);

    /**
     * Constructs a varint from an array of raw bytes, which are
     * meant to be an already encoded unsigned varint
     * @param varint_bytes an array of bytes representing an unsigned varint
     */
    explicit UVarint(gsl::span<const uint8_t> varint_bytes);

    UVarint() = default;
    UVarint(const UVarint &var) = default;
    UVarint(UVarint &&var) = default;

    /**
     * Converts a varint back to a usual unsigned integer.
     * @return an integer previously encoded to the varint
     */
    uint64_t toUInt64() const;

    /**
     * @return an array view to raw bytes of the stored varint
     */
    const std::vector<uint8_t> toBytes() const;

    /**
     * Assigns the varint to an unsigned integer, encoding the latter
     * @param n the integer to encode and store
     * @return this varint
     */
    UVarint &operator=(uint64_t n);

    /**
     * @return the number of bytes currently stored in a varint
     */
    size_t size() const;

    /**
     * @param varint_bytes an array with a raw byte representation of a varint
     * @return the size of the varint stored in the array, if its content is a
     * valid varint. Otherwise, the result is undefined
     */
    static size_t calculateSize(gsl::span<const uint8_t> varint_bytes);

   private:
    std::vector<uint8_t> bytes_;
  };

}  // namespace libp2p::multi

#endif  // KAGOME_VARINT_HPP
