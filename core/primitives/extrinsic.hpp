/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_EXTRINSIC_HPP
#define KAGOME_PRIMITIVES_EXTRINSIC_HPP

#include <iostream>

#include "common/buffer.hpp"

namespace kagome::primitives {
  /**
   * @brief Extrinsic class represents extrinsic
   */
  struct Extrinsic {
    kagome::common::Buffer data;  ///< extrinsic content as byte array
  };

  /**
   * @brief comparison operator for extrinsics
   * @param e1 left extrinsic value
   * @param e2 right extrinsic value
   * @return true if equal false otherwise
   */
  bool inline operator==(const Extrinsic &e1, const Extrinsic &e2) {
    return e1.data == e2.data;
  }

  /**
   * @brief outputs object of type Extrinisic to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <
      class Stream,
      typename S = std::enable_if_t<!std::is_same_v<::std::ostream, Stream>>>
  Stream &operator<<(Stream &s, const Extrinsic &v) {
    return s << v.data.toVector();
  }

  /**
   * @brief decodes object of type Extrinisic from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator>>(Stream &s, Extrinsic &v) {
    return s >> v.data;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_EXTRINSIC_HPP
