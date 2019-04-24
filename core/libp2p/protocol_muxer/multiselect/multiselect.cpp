/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

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

  Multiselect::~Multiselect() = default;

  void Multiselect::addProtocol(const multi::Multistream &protocol) {
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
    using MessageType =
        MultiselectCommunicator::MultiselectMessage::MessageType;

    auto message_opt = communicator_.parseMessage(response);
    if (!message_opt) {
      log_->error("cannot parse message, received from the stream");
      // response by sending ls; this can still allow us to continue negotiation
      sendLsMsg(std::move(stream_state));
      return;
    }

    auto message = *message_opt;
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
    }
  }

  void Multiselect::handleOpeningMsg(StreamState stream_state) const {
    // if we receive an opening message, don't look, which message was sent by
    // us previously, just respond with ls; it will always work
    sendLsMsg(std::move(stream_state));
  }

  void Multiselect::handleProtocolMsg(const multi::Multistream &protocol,
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
        // we asked for a list of supported protocols, and the other side
        // responded with only one; if it's not available on our side, send na?
        // otherwise, negotiation is finished
        if (std::find(supported_protocols_.begin(), supported_protocols_.end(),
                      protocol)
            != supported_protocols_.end()) {
          stream_state.proto_callback_(protocol);
          sendProtocolMsg(protocol, false, std::move(stream_state));
          return;
        }
        stream_state.proto_callback_(MultiselectErrors::NEGOTIATION_FAILED);
        sendNaMsg(std::move(stream_state));
        return;
      case Status::NA_SENT:
        // unexpected request-response combination; send ls
        sendLsMsg(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleProtocolsMsg(
      const std::vector<multi::Multistream> &protocols,
      StreamState stream_state) const {
    using Status = StreamState::NegotiationStatus;

    switch (stream_state.status_) {
      case Status::OPENING_SENT:
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::NA_SENT:
        // unexpected request-response combinations; send ls
        sendLsMsg(std::move(stream_state));
        return;
      case Status::LS_SENT:
        // if any of the received protocols is supported by our side, choose it;
        // send NA otherwise?
        for (const auto &proto : protocols) {
          // as size of vectors should be around 10 or less, we can use O(n*n)
          // approach
          if (std::find(supported_protocols_.begin(),
                        supported_protocols_.end(), proto)
              != supported_protocols_.end()) {
            // the protocol is found; send it as our choice
            sendProtocolMsg(proto, true, std::move(stream_state));
            return;
          }
        }
        stream_state.proto_callback_(MultiselectErrors::NEGOTIATION_FAILED);
        sendNaMsg(std::move(stream_state));
        return;
    }
  }

  void Multiselect::handleLsMsg(StreamState stream_state) const {
    // respond with a list of protocols, supported by us
    sendProtocolsMsg(gsl::span<const multi::Multistream>(supported_protocols_),
                     std::move(stream_state));
  }

  void Multiselect::handleNaMsg(StreamState stream_state) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    sendLsMsg(std::move(stream_state));
  }

  void Multiselect::sendOpeningMsg(StreamState stream_state) const {
    stream_state.stream_.get().writeAsync(
        communicator_.openingMsg(),
        [t = shared_from_this(), stream_state = std::move(stream_state)](
            const std::error_code &ec, size_t n) mutable {
          if (ec) {
            t->log_->error("cannot send an opening message: {}", ec.value());
            stream_state.proto_callback_(ec);
            return;
          }
          stream_state.status_ = StreamState::NegotiationStatus::OPENING_SENT;
          t->readResponse(std::move(stream_state));
        });
  }

  void Multiselect::sendProtocolMsg(const multi::Multistream &protocol,
                                    bool wait_for_response,
                                    StreamState stream_state) const {
    if (!wait_for_response) {
      stream_state.stream_.get().writeAsync(
          communicator_.protocolMsg(protocol));
      return;
    }
    stream_state.stream_.get().writeAsync(
        communicator_.protocolMsg(protocol),
        [t = shared_from_this(), stream_state = std::move(stream_state)](
            const std::error_code &ec, size_t n) mutable {
          if (ec) {
            t->log_->error("cannot send a protocol message: {}", ec.value());
            stream_state.proto_callback_(ec);
            return;
          }
          stream_state.status_ = StreamState::NegotiationStatus::PROTOCOL_SENT;
          t->readResponse(std::move(stream_state));
        });
  }

  void Multiselect::sendProtocolsMsg(gsl::span<const multi::Multistream> protocols,
                                     StreamState stream_state) const {
    stream_state.stream_.get().writeAsync(
        communicator_.protocolsMsg(protocols),
        [t = shared_from_this(), stream_state = std::move(stream_state)](
            const std::error_code &ec, size_t n) mutable {
          if (ec) {
            t->log_->error("cannot send a protocols message: {}", ec.value());
            stream_state.proto_callback_(ec);
            return;
          }
          stream_state.status_ = StreamState::NegotiationStatus::PROTOCOLS_SENT;
          t->readResponse(std::move(stream_state));
        });
  }

  void Multiselect::sendLsMsg(StreamState stream_state) const {
    stream_state.stream_.get().writeAsync(
        communicator_.lsMsg(),
        [t = shared_from_this(), stream_state = std::move(stream_state)](
            const std::error_code &ec, size_t n) mutable {
          if (ec) {
            t->log_->error("cannot send an ls message: {}", ec.value());
            stream_state.proto_callback_(ec);
            return;
          }
          stream_state.status_ = StreamState::NegotiationStatus::LS_SENT;
          t->readResponse(std::move(stream_state));
        });
  }

  void Multiselect::sendNaMsg(StreamState stream_state) const {
    stream_state.stream_.get().writeAsync(
        communicator_.naMsg(),
        [t = shared_from_this(), stream_state = std::move(stream_state)](
            const std::error_code &ec, size_t n) mutable {
          if (ec) {
            t->log_->error("cannot send an na message: {}", ec.value());
            stream_state.proto_callback_(ec);
            return;
          }
          stream_state.status_ = StreamState::NegotiationStatus::NA_SENT;
          t->readResponse(std::move(stream_state));
        });
  }
}  // namespace libp2p::protocol_muxer
