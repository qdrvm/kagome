/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_CLIENT_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_CLIENT_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class Client
   * @brief Abstraction of a single (possibly remote) Grandpa Server (we are the
   * client).
   */
  struct Client {
    virtual ~Client() = default;

    virtual void precommit(Precommit pc) = 0;
    virtual void prevote(Prevote pv) = 0;
    virtual void primaryPropose(PrimaryPropose pv) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_CLIENT_HPP
