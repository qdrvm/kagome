/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "consensus/grandpa/grandpa_context.hpp"
#include "network/types/grandpa_message.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class CatchUpObserver
   * @brief observes incoming catch-up messages. Abstraction of a network.
   */
  struct CatchUpObserver {
    virtual ~CatchUpObserver() = default;

    /**
     * Handler of grandpa catch-up-request messages
     * @param msg catch-up-request messages
     */
    virtual void onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                  network::CatchUpRequest &&msg) = 0;

    /**
     * Handler of grandpa catch-up-response messages
     * @param msg catch-up-response messages
     */
    virtual void onCatchUpResponse(
        std::optional<std::shared_ptr<GrandpaContext>> &&existed_context,
        const libp2p::peer::PeerId &peer_id,
        const network::CatchUpResponse &msg) = 0;
  };

}  // namespace kagome::consensus::grandpa
