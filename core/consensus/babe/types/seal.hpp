/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/sr25519_types.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::babe {
  /**
   * Basically a signature of the block's header
   */
  struct Seal {
    SCALE_TIE(1);

    /// Sig_sr25519(Blake2s(block_header))
    crypto::Sr25519Signature signature;
  };
}  // namespace kagome::consensus::babe
