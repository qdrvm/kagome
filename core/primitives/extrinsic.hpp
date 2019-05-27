/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_EXTRINSIC_HPP
#define KAGOME_PRIMITIVES_EXTRINSIC_HPP

#include "common/buffer.hpp"

namespace kagome::primitives {
  /**
   * @brief Extrinsic class represents extrinsic
   */
  struct Extrinsic {
    kagome::common::Buffer data;  ///< extrinsic content as byte array
  };

  /**
   * @brief outputs object of type Extrinisic to stream
   * @tparam Stream stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream>
  Stream &operator<<(Stream &s, const Extrinsic &v) {
    return s.putBuffer(v.data);
  }
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_EXTRINSIC_HPP
