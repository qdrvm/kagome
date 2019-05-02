/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STATE_HPP
#define KAGOME_CONNECTION_STATE_HPP

#include <functional>
#include <memory>
#include <optional>

#include <boost/asio/streambuf.hpp>
#include "common/buffer.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"
#include "libp2p/transport/connection.hpp"

namespace libp2p::protocol_muxer {
  class Multiselect;

  /**
   * Stores current state of protocol negotiation over the connection
   */
  struct ConnectionState {
    enum class NegotiationStatus {
      NOTHING_SENT,
      OPENING_SENT,
      PROTOCOL_SENT,
      PROTOCOLS_SENT,
      LS_SENT,
      NA_SENT
    };

    enum class NegotiationRound { ENCRYPTION, MULTIPLEXER, STREAM };

    /// connection, over which we are negotiating
    std::shared_ptr<transport::Connection> connection_;

    /// callback, which is to be called, when a protocol is established
    ProtocolMuxer::ChosenProtocolsCallback proto_callback_;

    /// current status of the negotiation
    NegotiationStatus status_ = NegotiationStatus::NOTHING_SENT;

    /// write buffer of this connection
    kagome::common::Buffer &write_buffer_;

    /// read buffer of this connection
    boost::asio::streambuf &read_buffer_;

    /// Multiselect instance, which spawned this connection state
    std::shared_ptr<Multiselect> multiselect_;

    /// round, on which the negotiation currently is; for now, there are two
    /// rounds to be passed
    NegotiationRound round_ = NegotiationRound::ENCRYPTION;

    /// encryption protocol, chosen as a result of negotiation round
    peer::Protocol chosen_encryption_proto_{};

    /// mupltiplexer protocol, chosen as a result of negotiation round
    peer::Protocol chosen_multiplexer_proto_{};

    /// initial stream protocol, chosen as a resul of negotiation round
    peer::Protocol chosen_stream_proto_{};
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_CONNECTION_STATE_HPP
