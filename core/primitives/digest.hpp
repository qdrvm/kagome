/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_DIGEST_HPP
#define KAGOME_CORE_PRIMITIVES_DIGEST_HPP

#include "common/buffer.hpp"

namespace kagome::primitives {

  /// @brief Digest is an implementation defined entity, it should be considered
  /// as an array of bytes
  using Digest = common::Buffer;

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_DIGEST_HPP
