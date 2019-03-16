/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_COMPACT_HPP
#define KAGOME_SCALE_COMPACT_HPP

#include <optional>

#include "common/result.hpp"
#include "common/scale/types.hpp"

namespace kagome::common::scale::compact {
  /**
   * @brief EncodeIntegerResult is encode integer result type
   */
  using EncodeIntegerResult = EncodeResult;

  /**
   * @brief result of decodeInteger operation
   */
  using DecodeIntegerResult = expected::Result<BigInteger, DecodeError>;

  /**
   * @brief encodes uint8_t using the scale compact codec
   * @param value source uint8_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(uint8_t value);

  /**
   * @brief encodes int8_t using the scale compact codec
   * @param value source int8_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(int8_t value);

  /**
   * @brief encodes uint16_t using the scale compact codec
   * @param value source uint16_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(uint16_t value);

  /**
   * @brief encodes int16_t using the scale compact codec
   * @param value source int16_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(int16_t value);

  /**
   * @brief encodes uint32_t using the scale compact codec
   * @param value source uint32_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(uint32_t value);

  /**
   * @brief encodes int32_t using the scale compact codec
   * @param value source int32_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(int32_t value);

  /**
   * @brief encodes int64_t value
   * @param value source uint64_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(uint64_t value);

  /**
   * @brief encodes int64_t value
   * @param value source int64_t value
   * @return encoded value or error
   */
  EncodeIntegerResult encodeInteger(int64_t value);

  /**
   * @brief compact-encodes BigInteger
   * @param value source BigInteger value
   * @return byte array result or error
   */
  EncodeIntegerResult encodeInteger(const BigInteger &value);

  /**
   * @brief function decodes compact-encoded integer
   * @param stream source stream
   * @return decoded BigInteger or error
   */
  DecodeIntegerResult decodeInteger(Stream &stream);
}  // namespace kagome::common::scale::compact

#endif  // KAGOME_SCALE_HPP
