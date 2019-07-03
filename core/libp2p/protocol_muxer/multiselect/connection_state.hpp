/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STATE_HPP
#define KAGOME_CONNECTION_STATE_HPP

#include <functional>
#include <memory>

#include <boost/asio/streambuf.hpp>
#include "common/buffer.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/protocol_muxer/multiselect/multiselect_error.hpp"
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

    /// protocols to be selected
    std::shared_ptr<std::vector<const peer::Protocol>> protocols_;

    /// callback, which is to be called, when a protocol is established over the
    /// connection
    ProtocolMuxer::ProtocolHandlerFunc proto_callback_;

    /// write buffer of this connection
    std::shared_ptr<kagome::common::Buffer> write_buffer_;

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
      connection_->write(*write_buffer_, write_buffer_->size(),
                         std::move(handler));
    }

    /**
     * Read from the underlying connection or stream
     * @param n - how much bytes to read
     * @param handler to be called, when the read is done
     * @note resul of read is going to be in the local read buffer
     */
    void read(size_t n,
              std::function<void(const outcome::result<void> &)> handler) {
      // if there are already enough bytes in our buffer, return them
      if (read_buffer_->size() >= n) {
        return handler(outcome::success());
      }

      auto to_read = n - read_buffer_->size();
      auto buf = std::make_shared<kagome::common::Buffer>(to_read, 0);
      return connection_->read(
          *buf, to_read,
          [self{shared_from_this()}, buf = std::move(buf),
           h = std::move(handler), to_read](auto &&res) {
            if (!res) {
              return h(res.error());
            }
            if (boost::asio::buffer_copy(
                    self->read_buffer_->prepare(to_read),
                    boost::asio::const_buffer(buf->data(), to_read))
                != to_read) {
              return h(MultiselectError::INTERNAL_ERROR);
            }
            self->read_buffer_->commit(to_read);
            h(outcome::success());
          });
    }

    ConnectionState(
        std::shared_ptr<basic::ReadWriter> conn,
        std::shared_ptr<std::vector<const peer::Protocol>> protocols,
        std::function<void(const outcome::result<peer::Protocol> &)> proto_cb,
        std::shared_ptr<kagome::common::Buffer> write_buffer,
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
