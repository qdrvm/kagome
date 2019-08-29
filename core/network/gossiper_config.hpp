/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_CONFIG_HPP
#define KAGOME_GOSSIPER_CONFIG_HPP

#include <cstddef>

namespace kagome::network {
  /**
   * Config of the Gossiper
   */
  struct GossiperConfig {
    enum class Strategy {
      RANDOM_N  // send a gossip message to some random number of peers
    };
    Strategy strategy;

    size_t random_n{};  // works with Strategy::RANDOM_N
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_CONFIG_HPP
