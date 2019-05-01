/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer,
                            Multiselect::MultiselectErrors, e) {
  using Errors = libp2p::protocol_muxer::Multiselect::MultiselectErrors;
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

  void Multiselect::negotiateServer(const stream::Stream &stream,
                                    ChosenProtocolsCallback protocol_callback) {
  }

  void Multiselect::negotiateClient(const stream::Stream &stream,
                                    ChosenProtocolsCallback protocol_callback) {
    // if no protocols are supported by this instance, there's no sense in
    // starting a negotiation process
    if (encryption_protocols_.empty() || multiplexer_protocols_.empty()) {
      protocol_callback(MultiselectErrors::NO_PROTOCOLS_SUPPORTED);
      return;
    }
    sendOpeningMsg(StreamState{std::cref(stream), std::move(protocol_callback),
                               StreamState::NegotiationStatus::OPENING_SENT});
  }

  void Multiselect::readResponse(StreamState stream_state) const {
    stream_state.stream_.get().readAsync(
        [t = shared_from_this(), stream_state = std::move(stream_state)](
            stream::Stream::NetworkMessageOutcome response) mutable {
          if (!response) {
            t->log_->error("cannot read a response from the stream: {}",
                           response.error().value());
            stream_state.proto_callback_(response.error());
            return;
          }
          t->processResponse(response.value(), std::move(stream_state));
        });
  }

  void Multiselect::processResponse(const kagome::common::Buffer &response,
                                    StreamState stream_state) const {
    using MessageType = MessageManager::MultiselectMessage::MessageType;

    auto message_res = MessageManager::parseMessage(response);
    if (!message_res) {
      log_->error("cannot parse message, received from the stream: {}",
                  message_res.error().value());
      // response by sending ls; this can still allow us to continue negotiation
      sendLsMsg(std::move(stream_state));
      return;
    }

    auto message = std::move(message_res.value());
    switch (message.type_) {
      case MessageType::OPENING:
        handleOpeningMsg(std::move(stream_state));
        return;
      case MessageType::PROTOCOL:
        handleProtocolMsg(message.protocols_[0], std::move(stream_state));
        return;
      case MessageType::PROTOCOLS:
        handleProtocolsMsg(message.protocols_, std::move(stream_state));
        return;
      case MessageType::LS:
        handleLsMsg(std::move(stream_state));
        return;
      case MessageType::NA:
        handleNaMsg(std::move(stream_state));
        return;
      default:
        log_->error("type of the message, returned by the parser, is unknown");
        sendLsMsg(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleOpeningMsg(StreamState stream_state) const {
    using Status = StreamState::NegotiationStatus;

    switch (stream_state.status_) {
      case Status::NOTHING_SENT:
        // we received an opening as a first message in this round; respond with
        // an opening as well
        sendOpeningMsg(std::move(stream_state));
        return;
      case Status::OPENING_SENT:
        // if opening is received as a response to ours, we can send ls to see
        // available protocols
        sendLsMsg(std::move(stream_state));
        return;
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::LS_SENT:
      case Status::NA_SENT:
        onUnexpectedRequestResponse(std::move(stream_state));
        return;
      default:
        onGarbagedStreamStatus(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleProtocolMsg(const Protocol &protocol,
                                      StreamState stream_state) const {
    using Status = StreamState::NegotiationStatus;

    switch (stream_state.status_) {
      case Status::OPENING_SENT:
        onProtocolAfterOpeningOrLs(std::move(stream_state), protocol);
        return;
      case Status::PROTOCOL_SENT:
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; round is finished
        negotiationRoundFinished(std::move(stream_state), protocol);
        return;
      case Status::PROTOCOLS_SENT:
        // the other side has chosen a protocol to communicate over; send an
        // ack, and round is finished
        sendProtocolAck(stream_state.stream_.get(), protocol);
        negotiationRoundFinished(std::move(stream_state), protocol);
        return;
      case Status::LS_SENT:
        onProtocolAfterOpeningOrLs(std::move(stream_state), protocol);
        return;
      case Status::NOTHING_SENT:
      case Status::NA_SENT:
        onUnexpectedRequestResponse(std::move(stream_state));
        return;
      default:
        onGarbagedStreamStatus(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleProtocolsMsg(const std::vector<Protocol> &protocols,
                                       StreamState stream_state) const {
    using Status = StreamState::NegotiationStatus;

    switch (stream_state.status_) {
      case Status::OPENING_SENT:
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::NA_SENT:
        onUnexpectedRequestResponse(std::move(stream_state));
        return;
      case Status::LS_SENT:
        onProtocolsAfterLs(std::move(stream_state), protocols);
        return;
      default:
        onGarbagedStreamStatus(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleLsMsg(StreamState stream_state) const {
    // respond with a list of protocols, supported by us
    switch (stream_state.round_) {
      case StreamState::NegotiationRound::ENCRYPTION:
        sendProtocolsMsg(encryption_protocols_, std::move(stream_state));
        return;
      case StreamState::NegotiationRound::MULTIPLEXER:
        sendProtocolsMsg(multiplexer_protocols_, std::move(stream_state));
        return;
      default:
        log_->error("stream state's round is set to garbage value");
        stream_state.proto_callback_(MultiselectErrors::INTERNAL_ERROR);
        return;
    }
  }

  void Multiselect::handleNaMsg(StreamState stream_state) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    sendLsMsg(std::move(stream_state));
  }

  void Multiselect::onProtocolAfterOpeningOrLs(
      StreamState stream_state, const peer::Protocol &protocol) const {
    // the other side wants to communicate over that protocol; if it's available
    // on our side, round is finished
    peer::Protocol common_protocol;
    switch (stream_state.round_) {
      case StreamState::NegotiationRound::ENCRYPTION:
        if (std::find(encryption_protocols_.begin(),
                      encryption_protocols_.end(), protocol)
            != encryption_protocols_.end()) {
          common_protocol = protocol;
        }
        break;
      case StreamState::NegotiationRound::MULTIPLEXER:
        if (std::find(multiplexer_protocols_.begin(),
                      multiplexer_protocols_.end(), protocol)
            != multiplexer_protocols_.end()) {
          common_protocol = protocol;
        }
        break;
      default:
        log_->error("stream state's round is set to garbage value");
        stream_state.proto_callback_(MultiselectErrors::INTERNAL_ERROR);
        return;
    }
    if (!common_protocol.empty()) {
      sendProtocolAck(stream_state.stream_.get(), protocol);
      negotiationRoundFinished(std::move(stream_state), protocol);
      return;
    }

    // if the protocol is not available, send na
    sendNaMsg(std::move(stream_state));
  }

  void Multiselect::onProtocolsAfterLs(
      StreamState stream_state,
      gsl::span<const Protocol> received_protocols) const {
    // TODO(akvinikym) [PRE-143] 30.04.19: store the protocols to the
    // protocol repository

    // if any of the received protocols is supported by our side, choose it;
    // fail otherwise
    peer::Protocol common_protocol;
    const auto &protocols_to_search =
        stream_state.round_ == StreamState::NegotiationRound::ENCRYPTION
        ? encryption_protocols_
        : multiplexer_protocols_;
    for (const auto &proto : protocols_to_search) {
      // as size of vectors should be around 10 or less, we can use O(n*n)
      // approach
      if (std::find(received_protocols.begin(), received_protocols.end(), proto)
          != received_protocols.end()) {
        // the protocol is found
        common_protocol = proto;
        break;
      }
    }

    if (!common_protocol.empty()) {
      sendProtocolMsg(common_protocol, std::move(stream_state));
      return;
    }
    stream_state.proto_callback_(MultiselectErrors::NEGOTIATION_FAILED);
  }

  void Multiselect::onUnexpectedRequestResponse(
      StreamState stream_state) const {
    log_->info("got a unexpected request-response combination - sending 'ls'");
    sendLsMsg(std::move(stream_state));
  }

  void Multiselect::onGarbagedStreamStatus(StreamState stream_state) const {
    log_->error("there is some garbage in stream state status");
    sendLsMsg(std::move(stream_state));
  }

  auto Multiselect::getWriteCallback(
      std::shared_ptr<const Multiselect> t, StreamState stream_state,
      StreamState::NegotiationStatus success_status,
      std::function<std::string(const std::error_code &ec)> error) const {
    return [t = std::move(t), stream_state = std::move(stream_state),
            success_status, error = std::move(error)](const std::error_code &ec,
                                                      size_t n) mutable {
      if (ec) {
        t->log_->error(error(ec));
        stream_state.proto_callback_(ec);
        return;
      }
      stream_state.status_ = success_status;
      t->readResponse(std::move(stream_state));
    };
  }

  void Multiselect::sendOpeningMsg(StreamState stream_state) const {
    // this is done to avoid use-after-move with stream_state
    auto stream = stream_state.stream_;
    stream.get().writeAsync(
        MessageManager::openingMsg(),
        getWriteCallback(shared_from_this(), std::move(stream_state),
                         StreamState::NegotiationStatus::OPENING_SENT,
                         [](const std::error_code &ec) {
                           return "cannot send an opening message: "
                               + std::to_string(ec.value());
                         }));
  }

  void Multiselect::sendProtocolMsg(const Protocol &protocol,
                                    StreamState stream_state) const {
    auto stream = stream_state.stream_;
    stream.get().writeAsync(
        MessageManager::protocolMsg(protocol),
        getWriteCallback(shared_from_this(), std::move(stream_state),
                         StreamState::NegotiationStatus::PROTOCOL_SENT,
                         [](const std::error_code &ec) {
                           return "cannot send a protocol message: "
                               + std::to_string(ec.value());
                         }));
  }

  void Multiselect::sendProtocolsMsg(gsl::span<const Protocol> protocols,
                                     StreamState stream_state) const {
    auto stream = stream_state.stream_;
    stream.get().writeAsync(
        MessageManager::protocolsMsg(protocols),
        getWriteCallback(shared_from_this(), std::move(stream_state),
                         StreamState::NegotiationStatus::PROTOCOLS_SENT,
                         [](const std::error_code &ec) {
                           return "cannot send protocols message: "
                               + std::to_string(ec.value());
                         }));
  }

  void Multiselect::sendLsMsg(StreamState stream_state) const {
    auto stream = stream_state.stream_;
    stream.get().writeAsync(
        MessageManager::lsMsg(),
        getWriteCallback(shared_from_this(), std::move(stream_state),
                         StreamState::NegotiationStatus::LS_SENT,
                         [](const std::error_code &ec) {
                           return "cannot send an ls message: "
                               + std::to_string(ec.value());
                         }));
  }

  void Multiselect::sendNaMsg(StreamState stream_state) const {
    auto stream = stream_state.stream_;
    stream.get().writeAsync(
        MessageManager::naMsg(),
        getWriteCallback(shared_from_this(), std::move(stream_state),
                         StreamState::NegotiationStatus::NA_SENT,
                         [](const std::error_code &ec) {
                           return "cannot send an na message: "
                               + std::to_string(ec.value());
                         }));
  }

  void Multiselect::sendProtocolAck(const stream::Stream &stream,
                                    const peer::Protocol &protocol) const {
    stream.writeAsync(MessageManager::protocolMsg(protocol));
  }

  void Multiselect::negotiationRoundFinished(
      StreamState stream_state, const Protocol &chosen_protocol) const {
    switch (stream_state.round_) {
      case StreamState::NegotiationRound::ENCRYPTION:
        // non-final round: continue negotiation, moving to the next round
        stream_state.round_ = StreamState::NegotiationRound::MULTIPLEXER;
        stream_state.chosen_encryption_proto_ = chosen_protocol;
        sendOpeningMsg(std::move(stream_state));
        return;
      case StreamState::NegotiationRound::MULTIPLEXER:
        // final round; negotiation is finished
        stream_state.chosen_multiplexer_proto_ = chosen_protocol;
        stream_state.proto_callback_(NegotiatedProtocols{
            std::move(stream_state.chosen_encryption_proto_),
            std::move(stream_state.chosen_multiplexer_proto_)});
        return;
      default:
        log_->error("stream state's round is set to garbage value");
        stream_state.proto_callback_(MultiselectErrors::INTERNAL_ERROR);
        return;
    }
  }
}  // namespace libp2p::protocol_muxer
