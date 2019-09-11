/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class Gossiper
   * @brief Gossip messages to the network via this class
   */
  struct Gossiper {
    virtual ~Gossiper() = default;

    virtual void precommit(Precommit pc) = 0;

    virtual void prevote(Prevote pv) = 0;

    virtual void primaryPropose(PrimaryPropose pv) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP
