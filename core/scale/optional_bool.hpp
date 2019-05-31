/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP
#define KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP

#include <optional>

/**
 * Optional bool is a special case of optional values in SCALE
 * Optional bools are encoded using only 1 byte
 * 0 means no value, 1 means false, 2 means true
 */

namespace kagome::scale {
  class ScaleEncoderStream;
  class ScaleDecoderStream;

  /**
   * @brief scale-encodes optional bool value
   * @param s reference to scale encoder stream
   * @param b optional bool value to encode
   * @return reference to stream
   */
  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const std::optional<bool> &b);

  /**
   * @brief scale-decodes optional bool value
   * @param s reference to scale decoder stream
   * @param b optional bool value to decode
   * @return reference to stream
   */
  ScaleDecoderStream &operator>>(ScaleDecoderStream &s, std::optional<bool> &b);
}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP
