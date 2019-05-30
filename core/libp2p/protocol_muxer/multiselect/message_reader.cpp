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

  std::optional<UVarint> getVarint(gsl::span<uint8_t> buffer) {
    return UVarint::create(buffer);
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;

  void MessageReader::readNextMessage(
      std::shared_ptr<ConnectionState> connection_state) {
    // start with reading a varint, which must be at the beginning of each
    // message
    readNextVarint(std::move(connection_state));
  }

  void MessageReader::readNextVarint(
      std::shared_ptr<ConnectionState> connection_state) {
    // we don't know a length of the varint, so gather it byte-by-byte
    std::vector<uint8_t> varint_bytes{};
    std::optional<UVarint> varint{std::nullopt};
    while (!varint) {
      auto varint_candidate_res = connection_state->read(1);
      if (!varint_candidate_res) {
        // something bad happened during read
        return processError(std::move(connection_state),
                            varint_candidate_res.error());
      }
      varint_bytes.push_back(varint_candidate_res.value()[0]);
      varint = getVarint(varint_bytes);
    }

    // that varint tells us, how much bytes to read next; do it
    readNextBytes(std::move(connection_state), varint->toUInt64(),
                  onReadLineCompleted);
  }

  void MessageReader::readNextBytes(
      std::shared_ptr<ConnectionState> connection_state, uint64_t bytes_to_read,
      const std::function<void(std::shared_ptr<ConnectionState>, Buffer)>
          &final_callback) {
    auto bytes_res = connection_state->read(bytes_to_read);
    if (!bytes_res) {
      processError(std::move(connection_state), bytes_res.error());
      return;
    }
    final_callback(std::move(connection_state),
                   Buffer{std::move(bytes_res.value())});
  }

  void MessageReader::onReadLineCompleted(
      std::shared_ptr<ConnectionState> connection_state, Buffer &&line_bytes) {
    // '/tls/1.3.0\n' - the shortest protocol, which could be found
    static constexpr size_t kShortestProtocolLength = 11;

    auto multiselect = connection_state->multiselect_;

    // firstly, try to match the message against constant messages
    auto const_msg_res = MessageManager::parseConstantMsg(line_bytes);
    if (const_msg_res) {
      multiselect->onReadCompleted(std::move(connection_state),
                                   std::move(const_msg_res.value()));
      return;
    }
    if (!const_msg_res
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

    // here, we assume that protocols header - two varints + '\n' - cannot be
    // longer or equal to the shortest protocol length; varints should be very
    // big for it to happen; thus, continue parsing depending on the length of
    // the current string
    if (line_bytes.size() < kShortestProtocolLength) {
      auto proto_header_res = MessageManager::parseProtocolsHeader(line_bytes);
      if (proto_header_res) {
        auto proto_header = proto_header_res.value();
        readNextBytes(
            std::move(connection_state), proto_header.size_of_protocols_,
            [proto_header](std::shared_ptr<ConnectionState> state, Buffer buf) {
              onReadProtocolsCompleted(std::move(state), std::move(buf),
                                       proto_header.size_of_protocols_,
                                       proto_header.number_of_protocols_);
            });
      } else {
        multiselect->onError(
            std::move(connection_state),
            "cannot parse message, received from the other side: "
                + proto_header_res.error().message());
      }
      return;
    }

    auto proto_res = MessageManager::parseProtocol(line_bytes);
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
      std::shared_ptr<ConnectionState> connection_state, Buffer protocols_bytes,
      uint64_t expected_protocols_size, uint64_t expected_protocols_number) {
    auto multiselect = connection_state->multiselect_;

    auto msg_res = MessageManager::parseProtocols(protocols_bytes.toVector(),
                                                  expected_protocols_number);
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

  void MessageReader::processError(
      std::shared_ptr<ConnectionState> connection_state,
      const std::error_code &ec) {
    auto multiselect = connection_state->multiselect_;
    multiselect->onError(std::move(connection_state),
                         "cannot read from the connection: " + ec.message(),
                         ec);
  }
}  // namespace libp2p::protocol_muxer
