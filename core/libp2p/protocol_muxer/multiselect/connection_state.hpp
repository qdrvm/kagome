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
#include "libp2p/connection/stream.hpp"
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

    /// connection, over which we are negotiating
    std::shared_ptr<basic::ReadWriter> connection_;

    ///
    std::shared_ptr<gsl::span<const peer::Protocol>> protocols_;

    /// callback, which is to be called, when a protocol is established over the
    /// connection
    ProtocolMuxer::ProtocolHandlerFunc proto_callback_;

    /// write buffer of this connection
    std::shared_ptr<boost::asio::streambuf> write_buffer_;

    /// read buffer of this connection
    std::shared_ptr<boost::asio::streambuf> read_buffer_;

    /// index of both buffers in Multiselect collection
    size_t buffers_index_;

    /// Multiselect instance, which spawned this connection state
    std::shared_ptr<Multiselect> multiselect_;

    /// current status of the negotiation
    NegotiationStatus status_ = NegotiationStatus::NOTHING_SENT;

    /**
     * Write to the underlying connection or stream
     * @param handler to be called, when the write is done
     * @note the function expects data to be written in the local write buffer
     */
    void write(basic::Writer::WriteCallbackFunc handler) {
      if (connection_ != nullptr) {
        auto span = gsl::make_span(
            static_cast<const uint8_t *>(write_buffer_->data().data()),
            write_buffer_->size());
        connection_->write(span, write_buffer_->size(), std::move(handler));
        return;
      }
    }

    /**
     * Read from the underlying connection or stream
     * @param n - how much bytes to read
     * @param handler to be called, when the read is done
     * @note resul of read is going to be in the local read buffer
     */
    void read(size_t n, basic::Reader::ReadCallbackFunc handler) {
      // if there are already enough bytes in our buffer, return them
      if (read_buffer_->size() >= n) {
        handler(boost::system::error_code{});
        return;
      }

      if (connection_ != nullptr) {
        std::vector<uint8_t> v;
        // check if it works
        v.resize(read_buffer_->size());
        boost::asio::buffer_copy(boost::asio::buffer(v), read_buffer_->data());
        connection_->read(v, read_buffer_->size(), std::move(handler));
        return;
      }
    }

    ConnectionState(
        std::shared_ptr<basic::ReadWriter> conn,
        std::shared_ptr<gsl::span<const peer::Protocol>> protocols,
        std::function<void(const outcome::result<peer::Protocol> &)> proto_cb,
        std::shared_ptr<boost::asio::streambuf> write_buffer,
        std::shared_ptr<boost::asio::streambuf> read_buffer,
        size_t buffers_index, std::shared_ptr<Multiselect> multiselect,
        NegotiationStatus status = NegotiationStatus::NOTHING_SENT)
        : connection_{std::move(conn)},
          protocols_{std::move(protocols)},
          proto_callback_{std::move(proto_cb)},
          write_buffer_{std::move(write_buffer)},
          read_buffer_{std::move(read_buffer)},
          buffers_index_{buffers_index},
          multiselect_{std::move(multiselect)},
          status_{status} {}
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_CONNECTION_STATE_HPP
