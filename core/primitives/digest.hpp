/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_DIGEST_HPP
#define KAGOME_CORE_PRIMITIVES_DIGEST_HPP

#include "common/buffer.hpp"

namespace kagome::primitives {
  /**
   * Digest is an implementation- and usage-defined entity, for example,
   * information, needed to verify the block
   */
  using Digest = common::Buffer;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_DIGEST_HPP
