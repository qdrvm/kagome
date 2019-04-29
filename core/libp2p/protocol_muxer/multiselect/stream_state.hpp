/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_STATE_HPP
#define KAGOME_STREAM_STATE_HPP

#include <functional>
#include <optional>

#include "libp2p/peer/peer_id.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Stores current state of protocol negotiation over the stream
   */
  struct StreamState {
    enum class NegotiationStatus {
      OPENING_SENT,
      PROTOCOL_SENT,
      PROTOCOLS_SENT,
      LS_SENT,
      NA_SENT
    };

    /// stream, over which we are negotiating
    std::reference_wrapper<const stream::Stream> stream_;

    /// callback, which is to be called, when a protocol is established
    ProtocolMuxer::ChosenProtocolCallback proto_callback_;

    /// current status of the negotiation
    NegotiationStatus status_;

    /// peer, with which we are negotiating
    std::optional<peer::PeerId> peer_id_{};

    /**
     * Set a peer id, if it's not set or if a new value is arrived
     * @param peer_id to be set
     */
    void setPeerId(std::optional<peer::PeerId> peer_id) {
      if (!peer_id_ || (peer_id_ && *peer_id_ != *peer_id)) {
        peer_id_ = std::move(*peer_id);
      }
    }
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_STREAM_STATE_HPP
