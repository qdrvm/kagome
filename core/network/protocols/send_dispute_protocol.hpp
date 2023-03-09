/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SENDDISPUTEPROTOCOL
#define KAGOME_NETWORK_SENDDISPUTEPROTOCOL

#include "network/protocol_base.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "outcome/outcome.hpp"

namespace kagome::network {

  struct DisputeRequest;

  using PeerId = libp2p::peer::PeerId;

  /**
   * @brief Class for communication via `/{chainType}/send_dispute/1` according
   * to send_dispute protocol specification
   * https://spec.polkadot.network/#net-msg-dispute-request
   * https://paritytech.github.io/polkadot/book/node/disputes/dispute-distribution.html
   */
  class SendDisputeProtocol : public virtual ProtocolBase {
   public:
    /**
     * @brief Make async request to peer and return response in callback
     * @param peer_id of a peer to make request to
     * @param dispute_request a request for dispute
     * @param response_handler a callback to call when response received
     */
    virtual void request(
        const PeerId &peer_id,
        DisputeRequest dispute_request,
        std::function<void(outcome::result<void>)> &&response_handler) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SENDDISPUTEPROTOCOL
