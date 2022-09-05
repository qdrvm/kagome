/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_JUSTIFICATION_HPP
#define KAGOME_JUSTIFICATION_HPP

#include "common/buffer.hpp"
#include "scale/tie.hpp"

namespace kagome::primitives {
  /**
   * Justification of the finalized block
   */
  struct Justification {
    SCALE_TIE(1);

    common::Buffer data;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_JUSTIFICATION_HPP
