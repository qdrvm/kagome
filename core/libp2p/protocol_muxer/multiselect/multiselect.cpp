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
      return "no protocols supported by this multiselect instance";
    case Errors::NEGOTIATION_FAILED:
      return "there are no protocols, supported by both sides of the "
             "connection";
  }
  return "unknown";
}

namespace libp2p::protocol_muxer {
  Multiselect::Multiselect(kagome::common::Logger logger)
      : log_{std::move(logger)} {}

  void Multiselect::addProtocol(const Protocol &protocol) {
    supported_protocols_.push_back(protocol);
  }

  void Multiselect::negotiateServer(const stream::Stream &stream,
                                    ChosenProtocolCallback protocol_callback) {}

  void Multiselect::negotiateClient(const stream::Stream &stream,
                                    ChosenProtocolCallback protocol_callback) {
    // if no protocols are supported by this instance, there's no sense in
    // starting a negotiation process
    if (supported_protocols_.empty()) {
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
    // if we receive an opening message, don't look, which message was sent by
    // us previously, just respond with ls; it will always work
    sendLsMsg(std::move(stream_state));
  }

  void Multiselect::handleProtocolMsg(const Protocol &protocol,
                                      StreamState stream_state) const {
    using Status = StreamState::NegotiationStatus;

    switch (stream_state.status_) {
      case Status::OPENING_SENT:
        // the other side wants to communicate over that protocol; if it's
        // available on our side, negotiation is finished
        if (std::find(supported_protocols_.begin(), supported_protocols_.end(),
                      protocol)
            != supported_protocols_.end()) {
          stream_state.proto_callback_(protocol);
          sendProtocolMsg(protocol, false, std::move(stream_state));
          return;
        }
        // if the protocol is not available, send ls
        sendLsMsg(std::move(stream_state));
        return;
      case Status::PROTOCOL_SENT:
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; negotiation is finished
        stream_state.proto_callback_(protocol);
        return;
      case Status::PROTOCOLS_SENT:
        // the other side has chosen a protocol to communicate over; send an
        // ack, and negotiation is finished
        stream_state.proto_callback_(protocol);
        sendProtocolMsg(protocol, false, std::move(stream_state));
        return;
      case Status::LS_SENT:
        onLsMsgSent(std::move(stream_state), gsl::make_span(&protocol, 1));
        return;
      case Status::NA_SENT:
        log_->info(
            "got a unexpected request-response combination - sending 'ls'");
        sendLsMsg(std::move(stream_state));
        return;
      default:
        log_->error("type of the message, returned by the parser, is unknown");
        sendLsMsg(std::move(stream_state));
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
        log_->info(
            "got a unexpected request-response combination - sending 'ls'");
        sendLsMsg(std::move(stream_state));
        return;
      case Status::LS_SENT:
        onLsMsgSent(std::move(stream_state), protocols);
        return;
      default:
        // in all unrecognized situations send ls
        log_->error("there is some garbage in stream state status");
        sendLsMsg(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleLsMsg(StreamState stream_state) const {
    // respond with a list of protocols, supported by us
    sendProtocolsMsg(supported_protocols_, std::move(stream_state));
  }

  void Multiselect::handleNaMsg(StreamState stream_state) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    sendLsMsg(std::move(stream_state));
  }

  void Multiselect::onLsMsgSent(
      StreamState stream_state,
      gsl::span<const Protocol> received_protocols) const {
    // TODO(akvinikym) [PRE-143] 30.04.19: store the protocols to the
    // protocol repository

    // if any of the received protocols is supported by our side, choose it;
    // send NA otherwise and close the connection?
    for (const auto &proto : received_protocols) {
      // as size of vectors should be around 10 or less, we can use O(n*n)
      // approach
      if (std::find(supported_protocols_.begin(), supported_protocols_.end(),
                    proto)
          != supported_protocols_.end()) {
        // the protocol is found; send it as our choice
        sendProtocolMsg(proto, true, std::move(stream_state));
        return;
      }
    }
    stream_state.proto_callback_(MultiselectErrors::NEGOTIATION_FAILED);
    sendNaMsg(std::move(stream_state));
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
                                    bool wait_for_response,
                                    StreamState stream_state) const {
    if (!wait_for_response) {
      stream_state.stream_.get().writeAsync(
          MessageManager::protocolMsg(protocol));
      return;
    }

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
}  // namespace libp2p::protocol_muxer
