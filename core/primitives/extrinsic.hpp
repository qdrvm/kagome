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
}  // namespace kagome::primitives

namespace kagome::scale {

  template <class Stream>
  Stream &operator<<(Stream &s, const primitives::Extrinsic &v) {
    return s.putBuffer(v.data);
  }
}  // namespace kagome::scale

#endif  // KAGOME_PRIMITIVES_EXTRINSIC_HPP
