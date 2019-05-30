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
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/basic/readwritecloser.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  class Multiselect;

  /**
   * Stores current state of protocol negotiation over the connection
   */
  struct ConnectionState : std::enable_shared_from_this<ConnectionState> {
    enum class NegotiationStatus {
      NOTHING_SENT,
      OPENING_SENT,
      PROTOCOL_SENT,
      PROTOCOLS_SENT,
      LS_SENT,
      NA_SENT
    };

    enum class NegotiationRound { ENCRYPTION, MULTIPLEXER, STREAM };

    /// connection or stream, over which we are negotiating
    std::shared_ptr<basic::ReadWriteCloser> connection_;

    /// callback, which is to be called, when a protocol is established over the
    /// connection or stream
    ProtocolMuxer::ChosenProtocolCallback proto_callback_;

    /// write buffer of this connection
    std::shared_ptr<kagome::common::Buffer> write_buffer_;

    /// index of both buffers in Multiselect collection
    size_t buffers_index_;

    /// Multiselect instance, which spawned this connection state
    std::shared_ptr<Multiselect> multiselect_;

    /// round, on which the negotiation currently is; for now, there are two
    /// rounds to be passed
    NegotiationRound round_;

    /// current status of the negotiation
    NegotiationStatus status_ = NegotiationStatus::NOTHING_SENT;

    /**
     * Write to the underlying connection or stream
     * @return nothing on success, error otherwise
     * @note the function expects data to be written in the local write buffer
     */
    outcome::result<void> write();

    /**
     * Read from the underlying connection or stream
     * @param n - how much bytes to read
     * @return read bytes in case of success, error otherwise
     */
    outcome::result<std::vector<uint8_t>> read(size_t n);

    /// ctor for the struct to be used in, for instance, make_shared
    ConnectionState(std::shared_ptr<basic::ReadWriteCloser> conn,
                    ProtocolMuxer::ChosenProtocolCallback proto_cb,
                    std::shared_ptr<kagome::common::Buffer> write_buffer,
                    size_t buffers_index,
                    std::shared_ptr<Multiselect> multiselect,
                    NegotiationRound round,
                    NegotiationStatus status = NegotiationStatus::NOTHING_SENT);
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_CONNECTION_STATE_HPP
