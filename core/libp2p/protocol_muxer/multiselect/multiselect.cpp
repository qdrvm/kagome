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
    app_protocols_.push_back(protocol);
  }

  outcome::result<peer::Protocol> Multiselect::negotiateEncryption(
      std::shared_ptr<connection::RawConnection> connection) {
    if (encryption_protocols_.empty()) {
      return MultiselectError::NO_PROTOCOLS_SUPPORTED;
    }
    return negotiate(connection, Round::ENCRYPTION);
  }

  outcome::result<peer::Protocol> Multiselect::negotiateMultiplexer(
      std::shared_ptr<connection::SecureConnection> connection) {
    if (multiplexer_protocols_.empty()) {
      return MultiselectError::NO_PROTOCOLS_SUPPORTED;
    }
    return negotiate(connection, Round::MUXER);
  }

  outcome::result<peer::Protocol> Multiselect::negotiateAppProtocol(
      std::shared_ptr<connection::Stream> stream) {
    if (app_protocols_.empty()) {
      return MultiselectError::NO_PROTOCOLS_SUPPORTED;
    }
    return negotiate(stream, Round::APP);
  }

  outcome::result<peer::Protocol> Multiselect::negotiate(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      const Round round) {
    using MessageType = MessageManager::MultiselectMessage::MessageType;

    OUTCOME_TRY(connection->write(MessageManager::openingMsg()));
    auto current_status = Status::OPENING_SENT;
    peer::Protocol previous_protocol{};

    while (current_status != Status::NEGOTIATION_SUCCESS
           || current_status != Status::NEGOTIATION_FAIL) {
      OUTCOME_TRY(response, MessageReader::readNextMessage(connection));
      switch (response.type) {
        case MessageType::OPENING: {
          OUTCOME_TRY(status, handleOpeningMsg(connection, current_status));
          current_status = status;
          break;
        }
        case MessageType::PROTOCOL: {
          OUTCOME_TRY(
              status,
              handleProtocolMsg(connection, response.protocols[0],
                                previous_protocol, current_status, round));
          current_status = status;
          break;
        }
        case MessageType::PROTOCOLS: {
          OUTCOME_TRY(status,
                      handleProtocolsMsg(connection, response.protocols,
                                         current_status, round));
          current_status = status;
          break;
        }
        case MessageType::LS: {
          OUTCOME_TRY(status, handleLsMsg(connection, round));
          current_status = status;
          break;
        }
        case MessageType::NA: {
          OUTCOME_TRY(status, handleNaMsg(connection));
          current_status = status;
          break;
        }
        default:
          return MultiselectError::INTERNAL_ERROR;
      }
      if (current_status == Status::PROTOCOL_SENT) {
        // memorize the protocol we sent
        previous_protocol = response.protocols[0];
      }
    }
    return finalizeNegotiation(current_status, previous_protocol);
  }

  outcome::result<peer::Protocol> Multiselect::finalizeNegotiation(
      const Status status, const peer::Protocol &protocol) {
    switch (status) {
      case Status::NEGOTIATION_SUCCESS:
        return protocol;
      case Status::NEGOTIATION_FAIL:
        return MultiselectError::NEGOTIATION_FAILED;
      default:
        // never should be here
        log_->critical("achieved state, which never should be achievable");
        return MultiselectError::INTERNAL_ERROR;
    }
  }

  outcome::result<Multiselect::Status> Multiselect::handleOpeningMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      Status status) const {
    switch (status) {
      case Status::OPENING_SENT: {
        // if opening is received as a response to ours, we can send ls to see
        // available protocols
        OUTCOME_TRY(connection->write(MessageManager::lsMsg()));
        return Status::LS_SENT;
      }
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::LS_SENT:
      case Status::NA_SENT:
        return onUnexpectedRequestResponse(connection);
      default:
        return onGarbagedStreamStatus(connection);
    }
  }

  outcome::result<Multiselect::Status> Multiselect::handleProtocolMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      const Protocol &protocol, const Protocol &prev_protocol, Status status,
      const Round round) {
    switch (status) {
      case Status::OPENING_SENT:
        return onProtocolAfterOpeningOrLs(connection, protocol, round);
      case Status::PROTOCOL_SENT: {
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; check it's the same we sent
        if (protocol == prev_protocol) {
          return Status::NEGOTIATION_SUCCESS;
        }
        // if not, the other side sent another protocol for some reason; respond
        // with ls
        OUTCOME_TRY(connection->write(MessageManager::lsMsg()));
        return Status::LS_SENT;
      }
      case Status::PROTOCOLS_SENT: {
        // the other side has chosen a protocol to communicate over; send an
        // ack, and round is finished
        OUTCOME_TRY(connection->write(MessageManager::protocolMsg(protocol)));
        return Status::NEGOTIATION_SUCCESS;
      }
      case Status::LS_SENT:
        return onProtocolAfterOpeningOrLs(connection, protocol, round);
      case Status::NA_SENT:
        return onUnexpectedRequestResponse(connection);
      default:
        return onGarbagedStreamStatus(connection);
    }
  }

  outcome::result<Multiselect::Status> Multiselect::handleProtocolsMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      const std::vector<Protocol> &protocols, Status status,
      const Round round) {
    switch (status) {
      case Status::LS_SENT:
        return onProtocolsAfterLs(connection, protocols, round);
      case Status::OPENING_SENT:
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::NA_SENT:
        return onUnexpectedRequestResponse(connection);
      default:
        return onGarbagedStreamStatus(connection);
    }
  }

  outcome::result<Multiselect::Status> Multiselect::handleLsMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      const Round round) {
    // respond with a list of protocols, supported by us
    auto protocols_to_send = getProtocolsByRound(round);
    if (protocols_to_send.empty()) {
      return MultiselectError::INTERNAL_ERROR;
    }
    OUTCOME_TRY(
        connection->write(MessageManager::protocolsMsg(protocols_to_send)));
    return Status::PROTOCOLS_SENT;
  }

  outcome::result<Multiselect::Status> Multiselect::handleNaMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    OUTCOME_TRY(connection->write(MessageManager::naMsg()));
    return Status::NA_SENT;
  }

  outcome::result<Multiselect::Status> Multiselect::onProtocolAfterOpeningOrLs(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      const peer::Protocol &protocol, const Round round) {
    // the other side wants to communicate over that protocol; if it's available
    // on our side, round is finished
    auto protocols_to_search = getProtocolsByRound(round);
    if (protocols_to_search.empty()) {
      return MultiselectError::INTERNAL_ERROR;
    }
    if (std::find(protocols_to_search.begin(), protocols_to_search.end(),
                  protocol)
        != protocols_to_search.end()) {
      OUTCOME_TRY(connection->write(MessageManager::protocolMsg(protocol)));
      return Status::NEGOTIATION_SUCCESS;
    }

    // if the protocol is not available, send na
    OUTCOME_TRY(connection->write(MessageManager::naMsg()));
    return Status::NA_SENT;
  }

  outcome::result<Multiselect::Status> Multiselect::onProtocolsAfterLs(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      gsl::span<const Protocol> received_protocols, const Round round) {
    // if any of the received protocols is supported by our side, choose it;
    // fail otherwise
    auto protocols_to_search = getProtocolsByRound(round);
    for (const auto &proto : protocols_to_search) {
      // as size of vectors should be around 10 or less, we can use O(n*n)
      // approach
      if (std::find(received_protocols.begin(), received_protocols.end(), proto)
          != received_protocols.end()) {
        // the protocol is found
        OUTCOME_TRY(connection->write(MessageManager::protocolMsg(proto)));
        return Status::PROTOCOL_SENT;
      }
    }
    return Status::NEGOTIATION_FAIL;
  }

  outcome::result<Multiselect::Status> Multiselect::onUnexpectedRequestResponse(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) const {
    log_->info("got a unexpected request-response combination - sending 'ls'");
    OUTCOME_TRY(connection->write(MessageManager::lsMsg()));
    return Status::LS_SENT;
  }

  outcome::result<Multiselect::Status> Multiselect::onGarbagedStreamStatus(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) const {
    log_->error("there is some garbage in stream state status - sending 'ls'");
    OUTCOME_TRY(connection->write(MessageManager::lsMsg()));
    return Status::LS_SENT;
  }

  gsl::span<const peer::Protocol> Multiselect::getProtocolsByRound(
      Round round) const {
    switch (round) {
      case Round::ENCRYPTION:
        return encryption_protocols_;
      case Round::MUXER:
        return multiplexer_protocols_;
      case Round::APP:
        return app_protocols_;
      default:
        log_->error("stream state's round is set to garbage value");
        return {};
    }
  }
}  // namespace libp2p::protocol_muxer
