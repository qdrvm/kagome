/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

#include <boost/asio/buffers_iterator.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer,
                            Multiselect::MultiselectError, e) {
  using Errors = libp2p::protocol_muxer::Multiselect::MultiselectError;
  switch (e) {
    case Errors::NO_PROTOCOLS_SUPPORTED:
      return "no protocols of at least one kind supported by this multiselect "
             "instance";
    case Errors::NEGOTIATION_FAILED:
      return "there are no protocols, supported by both sides of the "
             "connection";
    case Errors::INTERNAL_ERROR:
      return "internal error happened in this multiselect instance";
  }
  return "unknown";
}

namespace libp2p::protocol_muxer {
  using peer::Protocol;

  Multiselect::Multiselect(kagome::common::Logger logger)
      : log_{std::move(logger)} {}

  void Multiselect::addEncryptionProtocol(const peer::Protocol &protocol) {
    encryption_protocols_.push_back(protocol);
  }

  void Multiselect::addMultiplexerProtocol(const peer::Protocol &protocol) {
    multiplexer_protocols_.push_back(protocol);
  }

  void Multiselect::addStreamProtocol(const peer::Protocol &protocol) {
    stream_protocols_.push_back(protocol);
  }

  void Multiselect::negotiateEncryption(
      std::shared_ptr<connection::RawConnection> connection,
      ChosenProtocolCallback protocol_callback) {
    if (encryption_protocols_.empty()) {
      protocol_callback(MultiselectError::NO_PROTOCOLS_SUPPORTED);
      return;
    }

    auto [write_buffer, index] = getWriteBuffer();
    MessageWriter::sendOpeningMsg(std::make_shared<ConnectionState>(
        std::move(connection), std::move(protocol_callback),
        std::move(write_buffer), index, shared_from_this(),
        ConnectionState::NegotiationRound::ENCRYPTION));
  }

  void Multiselect::negotiateMultiplexer(
      std::shared_ptr<connection::SecureConnection> connection,
      ChosenProtocolCallback protocol_callback) {
    if (multiplexer_protocols_.empty()) {
      protocol_callback(MultiselectError::NO_PROTOCOLS_SUPPORTED);
      return;
    }

    auto [write_buffer, index] = getWriteBuffer();
    MessageWriter::sendOpeningMsg(std::make_shared<ConnectionState>(
        std::move(connection), std::move(protocol_callback),
        std::move(write_buffer), index, shared_from_this(),
        ConnectionState::NegotiationRound::MULTIPLEXER));
  }

  void Multiselect::negotiateStream(std::shared_ptr<connection::Stream> stream,
                                    ChosenProtocolCallback protocol_callback) {
    if (stream_protocols_.empty()) {
      protocol_callback(MultiselectError::NO_PROTOCOLS_SUPPORTED);
      return;
    }

    auto [write_buffer, index] = getWriteBuffer();
    MessageWriter::sendOpeningMsg(std::make_shared<ConnectionState>(
        std::move(stream), std::move(protocol_callback),
        std::move(write_buffer), index, shared_from_this(),
        ConnectionState::NegotiationRound::STREAM));
  }

  void Multiselect::onWriteCompleted(
      std::shared_ptr<ConnectionState> connection_state) const {
    MessageReader::readNextMessage(std::move(connection_state));
  }

  void Multiselect::onWriteAckCompleted(
      std::shared_ptr<ConnectionState> connection_state,
      const Protocol &protocol) {
    negotiationRoundFinished(std::move(connection_state), protocol);
  }

  void Multiselect::onReadCompleted(
      std::shared_ptr<ConnectionState> connection_state,
      MessageManager::MultiselectMessage msg) {
    processResponse(std::move(connection_state), std::move(msg));
  }

  void Multiselect::onError(std::shared_ptr<ConnectionState> connection_state,
                            std::string_view error) {
    onError(std::move(connection_state), error,
            MultiselectError::INTERNAL_ERROR);
  }

  void Multiselect::onError(std::shared_ptr<ConnectionState> connection_state,
                            std::string_view error, const std::error_code &ec) {
    log_->error(error);
    negotiationRoundFailed(std::move(connection_state), ec);
  }

  void Multiselect::processResponse(
      std::shared_ptr<ConnectionState> connection_state,
      MessageManager::MultiselectMessage msg) {
    using MessageType = MessageManager::MultiselectMessage::MessageType;

    switch (msg.type_) {
      case MessageType::OPENING:
        handleOpeningMsg(std::move(connection_state));
        return;
      case MessageType::PROTOCOL:
        handleProtocolMsg(msg.protocols_[0], std::move(connection_state));
        return;
      case MessageType::PROTOCOLS:
        handleProtocolsMsg(msg.protocols_, std::move(connection_state));
        return;
      case MessageType::LS:
        handleLsMsg(std::move(connection_state));
        return;
      case MessageType::NA:
        handleNaMsg(std::move(connection_state));
        return;
      default:
        log_->error("type of the message, returned by the parser, is unknown");
        MessageWriter::sendLsMsg(std::move(connection_state));
        return;
    }
  }

  void Multiselect::handleOpeningMsg(
      std::shared_ptr<ConnectionState> connection_state) const {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status_) {
      case Status::NOTHING_SENT:
        // we received an opening as a first message in this round; respond with
        // an opening as well
        MessageWriter::sendOpeningMsg(std::move(connection_state));
        return;
      case Status::OPENING_SENT:
        // if opening is received as a response to ours, we can send ls to see
        // available protocols
        MessageWriter::sendLsMsg(std::move(connection_state));
        return;
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::LS_SENT:
      case Status::NA_SENT:
        onUnexpectedRequestResponse(std::move(connection_state));
        return;
      default:
        onGarbagedStreamStatus(std::move(connection_state));
        return;
    }
  }

  void Multiselect::handleProtocolMsg(
      const Protocol &protocol,
      std::shared_ptr<ConnectionState> connection_state) {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status_) {
      case Status::OPENING_SENT:
        onProtocolAfterOpeningOrLs(std::move(connection_state), protocol);
        return;
      case Status::PROTOCOL_SENT:
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; round is finished
        negotiationRoundFinished(std::move(connection_state), protocol);
        return;
      case Status::PROTOCOLS_SENT:
        // the other side has chosen a protocol to communicate over; send an
        // ack, and round is finished
        MessageWriter::sendProtocolAck(std::move(connection_state), protocol);
        return;
      case Status::LS_SENT:
        onProtocolAfterOpeningOrLs(std::move(connection_state), protocol);
        return;
      case Status::NOTHING_SENT:
      case Status::NA_SENT:
        onUnexpectedRequestResponse(std::move(connection_state));
        return;
      default:
        onGarbagedStreamStatus(std::move(connection_state));
        return;
    }
  }

  void Multiselect::handleProtocolsMsg(
      const std::vector<Protocol> &protocols,
      std::shared_ptr<ConnectionState> connection_state) {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status_) {
      case Status::OPENING_SENT:
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::NA_SENT:
        onUnexpectedRequestResponse(std::move(connection_state));
        return;
      case Status::LS_SENT:
        onProtocolsAfterLs(std::move(connection_state), protocols);
        return;
      default:
        onGarbagedStreamStatus(std::move(connection_state));
        return;
    }
  }

  void Multiselect::handleLsMsg(
      std::shared_ptr<ConnectionState> connection_state) {
    // respond with a list of protocols, supported by us
    auto protocols_to_send = getProtocolsByRound(connection_state->round_);
    if (protocols_to_send.empty()) {
      negotiationRoundFailed(std::move(connection_state),
                             MultiselectError::INTERNAL_ERROR);
      return;
    }
    MessageWriter::sendProtocolsMsg(protocols_to_send,
                                    std::move(connection_state));
  }

  void Multiselect::handleNaMsg(
      std::shared_ptr<ConnectionState> connection_state) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    MessageWriter::sendLsMsg(std::move(connection_state));
  }

  void Multiselect::onProtocolAfterOpeningOrLs(
      std::shared_ptr<ConnectionState> connection_state,
      const peer::Protocol &protocol) {
    // the other side wants to communicate over that protocol; if it's available
    // on our side, round is finished
    auto protocols_to_search = getProtocolsByRound(connection_state->round_);
    if (protocols_to_search.empty()) {
      negotiationRoundFailed(std::move(connection_state),
                             MultiselectError::INTERNAL_ERROR);
      return;
    }
    if (std::find(protocols_to_search.begin(), protocols_to_search.end(),
                  protocol)
        != protocols_to_search.end()) {
      MessageWriter::sendProtocolAck(std::move(connection_state), protocol);
      return;
    }

    // if the protocol is not available, send na
    MessageWriter::sendNaMsg(std::move(connection_state));
  }

  void Multiselect::onProtocolsAfterLs(
      std::shared_ptr<ConnectionState> connection_state,
      gsl::span<const Protocol> received_protocols) {
    // if any of the received protocols is supported by our side, choose it;
    // fail otherwise
    auto protocols_to_search = getProtocolsByRound(connection_state->round_);
    for (const auto &proto : protocols_to_search) {
      // as size of vectors should be around 10 or less, we can use O(n*n)
      // approach
      if (std::find(received_protocols.begin(), received_protocols.end(), proto)
          != received_protocols.end()) {
        // the protocol is found
        MessageWriter::sendProtocolMsg(proto, std::move(connection_state));
        return;
      }
    }

    negotiationRoundFailed(std::move(connection_state),
                           MultiselectError::NEGOTIATION_FAILED);
  }

  void Multiselect::onUnexpectedRequestResponse(
      std::shared_ptr<ConnectionState> connection_state) const {
    log_->info("got a unexpected request-response combination - sending 'ls'");
    MessageWriter::sendLsMsg(std::move(connection_state));
  }

  void Multiselect::onGarbagedStreamStatus(
      std::shared_ptr<ConnectionState> connection_state) const {
    log_->error("there is some garbage in stream state status");
    MessageWriter::sendLsMsg(std::move(connection_state));
  }

  void Multiselect::negotiationRoundFinished(
      std::shared_ptr<ConnectionState> connection_state,  // NOLINT
      const Protocol &chosen_protocol) {
    switch (connection_state->round_) {
      case ConnectionState::NegotiationRound::ENCRYPTION:
      case ConnectionState::NegotiationRound::MULTIPLEXER:
      case ConnectionState::NegotiationRound::STREAM:
        connection_state->proto_callback_(chosen_protocol);
        break;
      default:
        log_->error("stream state's round is set to garbage value");
        // we don't know, what round the connection has; check callback manually
        if (connection_state->proto_callback_ != nullptr) {
          connection_state->proto_callback_(MultiselectError::INTERNAL_ERROR);
        } else {
          log_->critical("connection state is in absolutely garbaged state");
          return;
        }
        break;
    }
    clearResources(*connection_state);
  }

  void Multiselect::negotiationRoundFailed(
      std::shared_ptr<ConnectionState> connection_state,  // NOLINT
      const std::error_code &ec) {
    switch (connection_state->round_) {
      case ConnectionState::NegotiationRound::ENCRYPTION:
      case ConnectionState::NegotiationRound::MULTIPLEXER:
      case ConnectionState::NegotiationRound::STREAM:
        connection_state->proto_callback_(ec);
        break;
      default:
        log_->error("stream state's round is set to garbage value");
        if (connection_state->proto_callback_ != nullptr) {
          connection_state->proto_callback_(ec);
        } else {
          log_->critical("connection state is in absolutely garbaged state");
          return;
        }
        break;
    }
    clearResources(*connection_state);
  }

  gsl::span<const peer::Protocol> Multiselect::getProtocolsByRound(
      ConnectionState::NegotiationRound round) const {
    switch (round) {
      case ConnectionState::NegotiationRound::ENCRYPTION:
        return encryption_protocols_;
      case ConnectionState::NegotiationRound::MULTIPLEXER:
        return multiplexer_protocols_;
      case ConnectionState::NegotiationRound::STREAM:
        return stream_protocols_;
      default:
        log_->error("stream state's round is set to garbage value");
        return {};
    }
  }

  std::pair<std::shared_ptr<kagome::common::Buffer>, size_t>
  Multiselect::getWriteBuffer() {
    if (!free_buffers_.empty()) {
      auto free_buffers_index = free_buffers_.front();
      free_buffers_.pop();
      return {write_buffers_[free_buffers_index], free_buffers_index};
    }
    return {
        write_buffers_.emplace_back(std::make_shared<kagome::common::Buffer>()),
        write_buffers_.size() - 1};
  }

  void Multiselect::clearResources(const ConnectionState &connection_state) {
    // clear the buffer
    connection_state.write_buffer_->clear();

    // add it to the pool of free buffers
    free_buffers_.push(connection_state.buffers_index_);
  }
}  // namespace libp2p::protocol_muxer
