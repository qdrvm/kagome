/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNCHRONIZER_CONFIG_HPP
#define KAGOME_SYNCHRONIZER_CONFIG_HPP

#include <cstdint>

namespace kagome::consensus {
  struct SynchronizerConfig {
    /// how much blocks we can send at once
    uint32_t max_request_blocks = 128;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_SYNCHRONIZER_CONFIG_HPP
