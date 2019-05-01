/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_STATE_HPP
#define KAGOME_STREAM_STATE_HPP

#include <functional>
#include <optional>

#include "libp2p/protocol_muxer/protocol_muxer.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Stores current state of protocol negotiation over the stream
   */
  struct StreamState {
    enum class NegotiationStatus {
      NOTHING_SENT,
      OPENING_SENT,
      PROTOCOL_SENT,
      PROTOCOLS_SENT,
      LS_SENT,
      NA_SENT
    };

    enum class NegotiationRound { ENCRYPTION, MULTIPLEXER };

    /// stream, over which we are negotiating
    std::reference_wrapper<const stream::Stream> stream_;

    /// callback, which is to be called, when a protocol is established
    ProtocolMuxer::ChosenProtocolsCallback proto_callback_;

    /// current status of the negotiation
    NegotiationStatus status_ = NegotiationStatus::NOTHING_SENT;

    /// round, on which the negotiation currently is; for now, there are two
    /// rounds to be passed
    NegotiationRound round_ = NegotiationRound::ENCRYPTION;

    /// encryption protocol, chosen as a result of negotiation round
    peer::Protocol chosen_encryption_proto_{};

    /// mupltiplexer protocol, chosen as a result of negotiation round
    peer::Protocol chosen_multiplexer_proto_{};
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_STREAM_STATE_HPP
