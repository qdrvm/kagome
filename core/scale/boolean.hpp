/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_BOOLEAN_HPP
#define KAGOME_SCALE_BOOLEAN_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/byte_stream.hpp"
#include "scale/types.hpp"

namespace kagome::scale::boolean {
  /**
   * @brief encodeBool function encodes bool value and puts it to output buffer
   * @param out buffer which receives encoded value
   * @param value source bool value
   */
  void encodeBool(bool value, common::Buffer &out);

  /**
   * @brief decodeBool decodes bool value
   * @param byte source byte stream
   * @return decoded bool value
   */
  outcome::result<bool> decodeBool(common::ByteStream &stream);

  /**
   * @brief encodeTribool function encodes tribool value
   * and puts it to output buffer
   * @param out buffer which receives encoded value
   * @param value source tribool value
   */
  void encodeTribool(tribool value, common::Buffer &out);

  /**
   * @brief decodeTristate decodes tribool value representation
   * @param stream source stream containing tribool value
   * @return  decoded tribool value
   */
  outcome::result<tribool> decodeTribool(common::ByteStream &stream);

}  // namespace kagome::scale::boolean

#endif  // KAGOME_SCALE_BOOLEAN_HPP
