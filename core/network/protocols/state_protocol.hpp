/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_STATEPROTOCOL
#define KAGOME_NETWORK_STATEPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/types/state_request.hpp"
#include "network/types/state_response.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::ProtocolName;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  /**
   * @brief Class for communication via `/{chainType}/state/2` according to
   * state protocol specification
   * https://spec.polkadot.network/#sect-msg-state-request
   */
  class StateProtocol : public virtual ProtocolBase {
   public:
    /**
     * @brief Make async request to peer and return response in callback
     * @param peer_id of a peer to make request to
     * @param state_request a request content
     * @param response_handler a callback to call when response received
     */
    virtual void request(const PeerId &peer_id,
                         StateRequest state_request,
                         std::function<void(outcome::result<StateResponse>)>
                             &&response_handler) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_STATEPROTOCOL
