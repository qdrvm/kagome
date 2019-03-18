/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_COMPACT_HPP
#define KAGOME_SCALE_COMPACT_HPP

#include <optional>

#include "common/buffer.hpp"
#include "common/result.hpp"
#include "common/scale/types.hpp"

namespace kagome::common::scale::compact {
  /**
   * @brief result of decodeInteger operation
   */
  using DecodeIntegerResult = expected::Result<BigInteger, DecodeError>;

//  /**
//   * @brief encodes uint8_t using the scale compact codec
//   * @param value source uint8_t value
//   * @return encoded value or error
//   */
//  template <class T>
//  EncodeResult encodeInteger(T value, Buffer &out);
//
//  /**
//   * @brief encodes int8_t using the scale compact codec
//   * @param value source int8_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(int8_t value, Buffer &out);
//
//  /**
//   * @brief encodes uint16_t using the scale compact codec
//   * @param value source uint16_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(uint16_t value, Buffer &out);
//
//  /**
//   * @brief encodes int16_t using the scale compact codec
//   * @param value source int16_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(int16_t value, Buffer &out);
//
//  /**
//   * @brief encodes uint32_t using the scale compact codec
//   * @param value source uint32_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(uint32_t value, Buffer &out);
//
//  /**
//   * @brief encodes int32_t using the scale compact codec
//   * @param value source int32_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(int32_t value, Buffer &out);
//
//  /**
//   * @brief encodes int64_t value
//   * @param value source uint64_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(uint64_t value, Buffer &out);
//
//  /**
//   * @brief encodes int64_t value
//   * @param value source int64_t value
//   * @return encoded value or error
//   */
//  EncodeResult encodeInteger(int64_t value, Buffer &out);

  /**
   * @brief compact-encodes BigInteger
   * @param value source BigInteger value
   * @return byte array result or error
   */
  EncodeResult encodeInteger(const BigInteger &value, Buffer &out);

  /**
   * @brief function decodes compact-encoded integer
   * @param stream source stream
   * @return decoded BigInteger or error
   */
  DecodeIntegerResult decodeInteger(Stream &stream);
}  // namespace kagome::common::scale::compact

#endif  // KAGOME_SCALE_HPP
