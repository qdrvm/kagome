/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include <iostream>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer,
                            Multiselect::MultiselectError, e) {
  using Errors = libp2p::protocol_muxer::Multiselect::MultiselectError;
  switch (e) {
    case Errors::PROTOCOLS_LIST_EMPTY:
      return "no protocols were provided";
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

  void Multiselect::selectOneOf(
      gsl::span<const peer::Protocol> supported_protocols,
      std::shared_ptr<basic::ReadWriter> connection, bool is_initiator,
      ProtocolMuxer::ProtocolHandlerFunc handler) {
    if (supported_protocols.empty()) {
      handler(MultiselectError::PROTOCOLS_LIST_EMPTY);
      return;
    }

    negotiate(connection, supported_protocols, is_initiator, handler);
  }

  void Multiselect::negotiate(
      const std::shared_ptr<basic::ReadWriter> &connection,
      gsl::span<const peer::Protocol> supported_protocols, bool is_initiator,
      ProtocolHandlerFunc handler) {
    auto [write_buffer, read_buffer, index] = getBuffers();

    if (is_initiator) {
      MessageWriter::sendOpeningMsg(std::make_shared<ConnectionState>(
          connection,
          std::make_shared<std::vector<const peer::Protocol>>(
              supported_protocols.begin(), supported_protocols.end()),
          handler, write_buffer, read_buffer, index, shared_from_this()));
    } else {
      MessageReader::readNextMessage(std::make_shared<ConnectionState>(
          connection,
          std::make_shared<std::vector<const peer::Protocol>>(
              supported_protocols.begin(), supported_protocols.end()),
          handler, write_buffer, read_buffer, index, shared_from_this(),
          ConnectionState::NegotiationStatus::NOTHING_SENT));
    }
  }
  void Multiselect::negotiationRoundFailed(
      const std::shared_ptr<ConnectionState> &connection_state,
      const std::error_code &ec) {
    connection_state->proto_callback_(ec);
    clearResources(connection_state);
  }

  void Multiselect::onWriteCompleted(
      std::shared_ptr<ConnectionState> connection_state) const {
    MessageReader::readNextMessage(std::move(connection_state));
  }

  void Multiselect::onWriteAckCompleted(
      const std::shared_ptr<ConnectionState> &connection_state,
      const Protocol &protocol) {
    negotiationRoundFinished(connection_state, protocol);
  }

  void Multiselect::onReadCompleted(
      std::shared_ptr<ConnectionState> connection_state,
      MessageManager::MultiselectMessage msg) {
    using MessageType = MessageManager::MultiselectMessage::MessageType;

    switch (msg.type) {
      case MessageType::OPENING:
        handleOpeningMsg(std::move(connection_state));
        return;
      case MessageType::PROTOCOL:
        handleProtocolMsg(msg.protocols[0], std::move(connection_state));
        return;
      case MessageType::PROTOCOLS:
        handleProtocolsMsg(msg.protocols, std::move(connection_state));
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
        log_->info(
            "got a unexpected request-response combination - sending 'ls'");
        MessageWriter::sendLsMsg(std::move(connection_state));
        return;
      default:
        log_->error("there is some garbage in stream state status");
        MessageWriter::sendLsMsg(std::move(connection_state));
        return;
    }
  }

  void Multiselect::handleProtocolMsg(
      const peer::Protocol &protocol,
      std::shared_ptr<ConnectionState> connection_state) {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status_) {
      case Status::OPENING_SENT:
        onProtocolAfterOpeningOrLs(std::move(connection_state), protocol);
        return;
      case Status::PROTOCOL_SENT:
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; round is finished
        negotiationRoundFinished(connection_state, protocol);
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
    auto protocols_to_send = connection_state->protocols_;
    if (protocols_to_send->empty()) {
      std::cerr << "here22" << std::endl;
      negotiationRoundFailed(connection_state,
                             MultiselectError::INTERNAL_ERROR);
      return;
    }
    MessageWriter::sendProtocolsMsg(*protocols_to_send,
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
    auto protocols_to_search = connection_state->protocols_;
    if (protocols_to_search->empty()) {
      std::cerr << "here11" << std::endl;
      negotiationRoundFailed(connection_state,
                             MultiselectError::INTERNAL_ERROR);
      return;
    }
    if (std::find(protocols_to_search->begin(), protocols_to_search->end(),
                  protocol)
        != protocols_to_search->end()) {
      MessageWriter::sendProtocolAck(std::move(connection_state), protocol);
      return;
    }

    // if the protocol is not available, send na
    MessageWriter::sendNaMsg(std::move(connection_state));
  }

  void Multiselect::onProtocolsAfterLs(
      std::shared_ptr<ConnectionState> connection_state,
      gsl::span<const peer::Protocol> received_protocols) {
    // TODO(akvinikym) [PRE-143] 30.04.19: store the protocols to the
    // protocol repository

    // if any of the received protocols is supported by our side, choose it;
    // fail otherwise
    auto protocols_to_search = connection_state->protocols_;
    for (const auto &proto : *protocols_to_search) {
      // as size of vectors should be around 10 or less, we can use O(n*n)
      // approach
      if (std::find(received_protocols.begin(), received_protocols.end(), proto)
          != received_protocols.end()) {
        // the protocol is found
        MessageWriter::sendProtocolMsg(proto, std::move(connection_state));
        return;
      }
    }

    negotiationRoundFailed(connection_state,
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
      const std::shared_ptr<ConnectionState> &connection_state,
      const Protocol &chosen_protocol) {
    connection_state->proto_callback_(chosen_protocol);
    clearResources(connection_state);
  }

  std::tuple<std::shared_ptr<kagome::common::Buffer>,
             std::shared_ptr<boost::asio::streambuf>, size_t>
  Multiselect::getBuffers() {
    if (!free_buffers_.empty()) {
      auto free_buffers_index = free_buffers_.front();
      free_buffers_.pop();
      return {write_buffers_[free_buffers_index],
              read_buffers_[free_buffers_index], free_buffers_index};
    }
    return {
        write_buffers_.emplace_back(std::make_shared<kagome::common::Buffer>()),
        read_buffers_.emplace_back(std::make_shared<boost::asio::streambuf>()),
        write_buffers_.size() - 1};
  }

  void Multiselect::clearResources(
      const std::shared_ptr<ConnectionState> &connection_state) {
    // add them to the pool of free buffers
    free_buffers_.push(connection_state->buffers_index_);
  }
}  // namespace libp2p::protocol_muxer
