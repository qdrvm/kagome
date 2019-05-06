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
#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/connection.hpp"

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

    /// either connection or stream can be set at a time

    /// connection, over which we are negotiating
    std::shared_ptr<transport::Connection> connection_;
    /// stream, over which we are negotiating
    std::unique_ptr<stream::Stream> stream_;

    /// callback, which is to be called, when a protocol is established
    ProtocolMuxer::ChosenProtocolCallback proto_callback_;

    /// write buffer of this connection
    std::shared_ptr<kagome::common::Buffer> write_buffer_;

    /// read buffer of this connection
    std::shared_ptr<boost::asio::streambuf> read_buffer_;

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
     * @param handler to be called, when the write is done
     * @note the function expects data to be written in the local write buffer
     */
    void write(std::function<basic::Writable::CompletionHandler> handler) {
      if (connection_ != nullptr) {
        connection_->asyncWrite(
            boost::asio::const_buffer{write_buffer_->toBytes(),
                                      write_buffer_->size()},
            std::move(handler));
        return;
      }
      stream_->writeAsync(*write_buffer_, std::move(handler));
    }

    /**
     * Read from the underlying connection or stream
     * @param n - how much bytes to read
     * @param handler to be called, when the read is done
     * @note resul of read is going to be in the local read buffer
     */
    void read(size_t n,
              std::function<basic::Readable::CompletionHandler> handler) {
      if (connection_ != nullptr) {
        connection_->asyncRead(*read_buffer_, n, std::move(handler));
        return;
      }
      stream_->readAsync(
          [t = shared_from_this(), handler = std::move(handler)](
              stream::Stream::NetworkMessageOutcome msg_res) mutable {
            if (msg_res) {
              // put the received message to our buffer
              std::ostream out(&*t->read_buffer_);
              std::string incoming_bytes(
                  msg_res.value().toBytes(),
                  msg_res.value().toBytes()
                      + msg_res.value().size());  // NOLINT
              out << incoming_bytes;
              handler(std::error_code{}, incoming_bytes.size());
            } else {
              handler(msg_res.error(), 0);
            }
          });
    }

    ConnectionState(std::shared_ptr<transport::Connection> conn,
                    std::unique_ptr<stream::Stream> stream,
                    ProtocolMuxer::ChosenProtocolCallback proto_cb,
                    std::shared_ptr<kagome::common::Buffer> write_buffer,
                    std::shared_ptr<boost::asio::streambuf> read_buffer,
                    size_t buffers_index,
                    std::shared_ptr<Multiselect> multiselect,
                    NegotiationRound round,
                    NegotiationStatus status = NegotiationStatus::NOTHING_SENT)
        : connection_{std::move(conn)},
          stream_{std::move(stream)},
          proto_callback_{std::move(proto_cb)},
          write_buffer_{std::move(write_buffer)},
          read_buffer_{std::move(read_buffer)},
          buffers_index_{buffers_index},
          multiselect_{std::move(multiselect)},
          round_{round},
          status_{status} {}
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_CONNECTION_STATE_HPP
