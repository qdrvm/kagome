/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE58CODEC_HPP
#define KAGOME_BASE58CODEC_HPP

#include <array>
#include <string>
#include <gsl/span>
#include <common/buffer.hpp>

#include "outcome/outcome.hpp"



namespace libp2p::multi {

  /**
   *  Base58 is a group of binary-to-text encoding schemes used to represent
   * large integers as alphanumeric text
   */
  class Base58Codec {
   public:

    /**
     * Enumeration of errors that might occur when decoding a base58 string
     */
    enum class DecodeError {
      kInvalidHighBit,
      kInvalidBase58Digit,
      kOutputTooBig
    };

    /**
     * encode a string into a base58 string (interpreting characters as bytes)
     * @return base58 representation of the string
     */
    static std::string encode(std::string_view str);

    /**
     * encode an array of bytes into a base58 string
     * @return the base58 string
     */
    static std::string encode(gsl::span<const uint8_t> bytes);

    /**
     * convert a base58 encoded string into a binary array
     * @return the binary array
     */
    static auto decode(std::string_view base58string) -> outcome::result<kagome::common::Buffer>;

   private:
    static size_t calculateDecodedSize(std::string_view base58string);

    constexpr static std::string_view b58digits_ordered =
        "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    constexpr static std::array<int16_t, 8 * 16> b58digits_map {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  -1, -1, -1, -1, -1, -1,
        -1, 9,  10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
        22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
        -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
        47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1
    };
  };

}  // namespace libp2p::multi

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi, Base58Codec::DecodeError);

#endif  // KAGOME_BASE58CODEC_HPP
