/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_reader.hpp"

#include <optional>

#include "libp2p/multi/uvarint.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"
#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

namespace {
  using libp2p::multi::UVarint;

  std::optional<UVarint> getVarint(boost::asio::streambuf &buffer) {
    return UVarint::createVarint(gsl::make_span(
        static_cast<const uint8_t *>(buffer.data().data()), buffer.size()));
  }
}  // namespace

namespace libp2p::protocol_muxer {
  void MessageReader::readNextMessage(ConnectionState connection_state) {
    // start with reading a varint, which must be at the beginning of each
    // message
    readNextVarint(std::move(connection_state));
  }

  void MessageReader::readNextVarint(ConnectionState connection_state) {
    // we don't know exact length of varint, so read byte-by-byte
    auto conn = connection_state.connection_;
    conn->asyncRead(connection_state.read_buffer_, 1,
                    [connection_state = std::move(connection_state)](
                        const std::error_code &ec, size_t n) mutable {
                      if (ec || n != 1) {
                        auto multiselect = connection_state.multiselect_;
                        multiselect->onError(
                            std::move(connection_state),
                            "cannot read from the connection: " + ec.message(),
                            ec);
                        return;
                      }
                      onReadVarintCompleted(std::move(connection_state));
                    });
  }

  void MessageReader::onReadVarintCompleted(ConnectionState connection_state) {
    auto varint_opt = getVarint(connection_state.read_buffer_);
    if (!varint_opt) {
      // no varint; continue reading
      readNextVarint(std::move(connection_state));
      return;
    }
    // we have length of the line to be read; do it
    connection_state.read_buffer_.consume(varint_opt->size());
    readNextBytes(
        std::move(connection_state), varint_opt->toUInt64(),
        [](ConnectionState state) { onReadLineCompleted(std::move(state)); });
  }

  void MessageReader::readNextBytes(
      ConnectionState connection_state, uint64_t bytes_to_read,
      std::function<void(ConnectionState)> final_callback) {
    auto conn = connection_state.connection_;
    conn->asyncRead(connection_state.read_buffer_, bytes_to_read,
                    [connection_state = std::move(connection_state),
                     bytes_to_read, final_callback = std::move(final_callback)](
                        const std::error_code &ec, size_t n) mutable {
                      if (ec || n != bytes_to_read) {
                        auto multiselect = connection_state.multiselect_;
                        multiselect->onError(
                            std::move(connection_state),
                            "cannot read from the connection: " + ec.message(),
                            ec);
                        return;
                      }
                      final_callback(std::move(connection_state));
                    });
  }

  void MessageReader::onReadLineCompleted(ConnectionState connection_state) {
    auto multiselect = connection_state.multiselect_;

    auto msg_span =
        gsl::make_span(static_cast<const uint8_t *>(
                           connection_state.read_buffer_.data().data()),
                       connection_state.read_buffer_.size());
    connection_state.read_buffer_.consume(msg_span.size());

    // firstly, try to match the message against constant messages
    auto const_msg_res = MessageManager::parseConstantMsg(msg_span);
    if (const_msg_res) {
      multiselect->onReadCompleted(std::move(connection_state),
                                   std::move(const_msg_res.value()));
      return;
    } else if (!const_msg_res
               && const_msg_res.error()
                   != MessageManager::ParseError::MSG_IS_ILL_FORMED) {
      // MSG_IS_ILL_FORMED allows us to continue parsing; otherwise, it's an
      // error
      multiselect->onError(
          std::move(connection_state),
          "cannot parse message, received from the other side: "
              + const_msg_res.error().message());
      return;
    }

    // secondly, try protocols header
    auto proto_header_res = MessageManager::parseProtocolsHeader(msg_span);
    if (proto_header_res) {
      readNextBytes(std::move(connection_state),
                    proto_header_res.value().size_of_protocols_,
                    [&proto_header_res](ConnectionState state) {
                      onReadProtocolsCompleted(
                          std::move(state),
                          proto_header_res.value().number_of_protocols_);
                    });
      return;
    }

    // finally, a single protocol message
    auto proto_res = MessageManager::parseProtocol(msg_span);
    if (!proto_res) {
      multiselect->onError(
          std::move(connection_state),
          "cannot parse message, received from the other side: "
              + proto_res.error().message());
      return;
    }

    multiselect->onReadCompleted(std::move(connection_state),
                                 std::move(proto_res.value()));
  }

  void MessageReader::onReadProtocolsCompleted(
      ConnectionState connection_state, uint64_t expected_protocols_number) {
    auto multiselect = connection_state.multiselect_;

    auto msg_res = MessageManager::parseProtocols(
        gsl::make_span(static_cast<const uint8_t *>(
                           connection_state.read_buffer_.data().data()),
                       connection_state.read_buffer_.size()),
        expected_protocols_number);
    connection_state.read_buffer_.consume(connection_state.read_buffer_.size());
    if (!msg_res) {
      multiselect->onError(
          std::move(connection_state),
          "cannot parse message, received from the other side: "
              + msg_res.error().message());
      return;
    }

    multiselect->onReadCompleted(std::move(connection_state),
                                 std::move(msg_res.value()));
  }
}  // namespace libp2p::protocol_muxer
