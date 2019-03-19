/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_BOOLEAN_HPP
#define KAGOME_SCALE_BOOLEAN_HPP

#include "common/buffer.hpp"
#include "common/scale/types.hpp"

namespace kagome::common::scale::boolean {
  /**
   * @brief encodeBool function encodes bool value and puts it to output buffer
   * @param out buffer which receives encoded value
   * @param value source bool value
   */
  void encodeBool(bool value, Buffer &out);

  /**
   * @brief DecodeBoolResult type is decodeBool function result type
   */
  using DecodeBoolResult = TypeDecodeResult<bool>;

  /**
   * @brief decodeBool decodes bool value
   * @param byte source byte stream
   * @return decoded bool value
   */
  DecodeBoolResult decodeBool(Stream &stream);

  /**
   * @brief encodeTribool function encodes tribool value
   * and puts it to output buffer
   * @param out buffer which receives encoded value
   * @param value source tribool value
   */
  void encodeTribool(tribool value, Buffer &out);

  using DecodeTriboolResult = TypeDecodeResult<tribool>;

  /**
   * @brief decodeTristate decodes tribool value representation
   * @param stream source stream containing tribool value
   * @return  decoded tribool value
   */
  DecodeTriboolResult decodeTribool(Stream &stream);
}  // namespace kagome::common::scale::boolean

#endif  // KAGOME_SCALE_BOOLEAN_HPP
