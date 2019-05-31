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

namespace {
  using libp2p::peer::Protocol;
  bool protocolIn(const Protocol &protocol,
                  gsl::span<const Protocol> protocols) {
    return std::find(protocols.begin(), protocols.end(), protocol)
        != protocols.end();
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using peer::Protocol;

  Multiselect::Multiselect(kagome::common::Logger logger)
      : log_{std::move(logger)} {}

  outcome::result<peer::Protocol> Multiselect::selectOneOf(
      gsl::span<const peer::Protocol> supported_protocols,
      std::shared_ptr<basic::ReadWriteCloser> connection,
      bool is_initiator) const {
    if (supported_protocols.empty()) {
      return MultiselectError::PROTOCOLS_LIST_EMPTY;
    }

    Status current_status;
    if (is_initiator) {
      OUTCOME_TRY(connection->write(MessageManager::openingMsg()));
      current_status = Status::OPENING_SENT;
    } else {
      current_status = Status::NOTHING_SENT;
    }

    return negotiate(connection, supported_protocols, current_status);
  }

  outcome::result<peer::Protocol> Multiselect::negotiate(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      gsl::span<const peer::Protocol> supported_protocols,
      Status current_status) const {
    using MessageType = MessageManager::MultiselectMessage::MessageType;

    peer::Protocol previous_protocol{};
    while (current_status != Status::NEGOTIATION_SUCCESS
           && current_status != Status::NEGOTIATION_FAIL) {
      OUTCOME_TRY(response, MessageReader::readNextMessage(connection));
      switch (response.type) {
        case MessageType::OPENING: {
          OUTCOME_TRY(status, handleOpeningMsg(connection, current_status));
          current_status = status;
          break;
        }
        case MessageType::PROTOCOL: {
          OUTCOME_TRY(status,
                      handleProtocolMsg(connection, supported_protocols,
                                        response.protocols[0],
                                        previous_protocol, current_status));
          current_status = status;
          if (current_status == Status::NEGOTIATION_SUCCESS) {
            previous_protocol = response.protocols[0];
          }
          break;
        }
        case MessageType::PROTOCOLS: {
          OUTCOME_TRY(status,
                      handleProtocolsMsg(connection, supported_protocols,
                                         response.protocols, current_status));
          current_status = status.first;
          if (current_status == Status::PROTOCOL_SENT) {
            previous_protocol = status.second;
          }
          break;
        }
        case MessageType::LS: {
          OUTCOME_TRY(status, handleLsMsg(connection, supported_protocols));
          current_status = status;
          if (current_status == Status::PROTOCOL_SENT) {
            // memorize the protocol we sent
            previous_protocol = response.protocols[0];
          }
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
    }
    return finalizeNegotiation(current_status, previous_protocol);
  }

  outcome::result<peer::Protocol> Multiselect::finalizeNegotiation(
      const Status status, const peer::Protocol &protocol) const {
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
      case Status::NOTHING_SENT: {
        // we received an opening from the other side; signalize we are here &
        // ready
        OUTCOME_TRY(connection->write(MessageManager::openingMsg()));
        return Status::OPENING_SENT;
      }
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
      gsl::span<const peer::Protocol> supported_protocols,
      const Protocol &protocol, const Protocol &prev_protocol,
      Status status) const {
    switch (status) {
      case Status::OPENING_SENT: {
        // if this protocol is available on our side, negotiation is finished
        if (protocolIn(protocol, supported_protocols)) {
          OUTCOME_TRY(connection->write(MessageManager::protocolMsg(protocol)));
          return Status::NEGOTIATION_SUCCESS;
        }
        OUTCOME_TRY(connection->write(MessageManager::naMsg()));
        return Status::NA_SENT;
      }
      case Status::PROTOCOL_SENT: {
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; check it's the same we sent
        if (protocol == prev_protocol) {
          return Status::NEGOTIATION_SUCCESS;
        }
        // if not, the other side sent another protocol for some reason
        return Status::NEGOTIATION_FAIL;
      }
      case Status::PROTOCOLS_SENT: {
        // the other side has chosen a protocol to communicate over; send an
        // ack after checking this protocol is really supported by us
        if (protocolIn(protocol, supported_protocols)) {
          OUTCOME_TRY(connection->write(MessageManager::protocolMsg(protocol)));
          return Status::NEGOTIATION_SUCCESS;
        }
        // again, something went wrong
        return Status::NEGOTIATION_FAIL;
      }
      case Status::NA_SENT: {
        // the other side sent another protocol we may support
        if (protocolIn(protocol, supported_protocols)) {
          // found a protocol - send it as out choice
          OUTCOME_TRY(connection->write(MessageManager::protocolMsg(protocol)));
          return Status::PROTOCOL_SENT;
        }
        OUTCOME_TRY(connection->write(MessageManager::naMsg()));
        return Status::NA_SENT;
      }
      case Status::LS_SENT:
      case Status::NOTHING_SENT:
        return onUnexpectedRequestResponse(connection);
      default:
        return onGarbagedStreamStatus(connection);
    }
  }

  outcome::result<std::pair<Multiselect::Status, peer::Protocol>>
  Multiselect::handleProtocolsMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      gsl::span<const peer::Protocol> supported_protocols,
      const std::vector<Protocol> &protocols, Status status) const {
    switch (status) {
      case Status::LS_SENT: {
        // if any of the received protocols is supported by our side, choose it
        for (const auto &proto : supported_protocols) {
          if (protocolIn(proto, protocols)) {
            // the protocol is found
            OUTCOME_TRY(connection->write(MessageManager::protocolMsg(proto)));
            return {Status::PROTOCOL_SENT, proto};
          }
        }
        return {Status::NEGOTIATION_FAIL, ""};
      }
      case Status::OPENING_SENT:
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::NOTHING_SENT:
      case Status::NA_SENT: {
        OUTCOME_TRY(h_status, onUnexpectedRequestResponse(connection));
        return {h_status, ""};
      }
      default: {
        OUTCOME_TRY(h_status, onGarbagedStreamStatus(connection));
        return {h_status, ""};
      }
    }
  }

  outcome::result<Multiselect::Status> Multiselect::handleLsMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection,
      gsl::span<const Protocol> supported_protocols) const {
    // respond with a list of protocols, supported by us
    if (supported_protocols.empty()) {
      return MultiselectError::INTERNAL_ERROR;
    }
    OUTCOME_TRY(
        connection->write(MessageManager::protocolsMsg(supported_protocols)));
    return Status::PROTOCOLS_SENT;
  }

  outcome::result<Multiselect::Status> Multiselect::handleNaMsg(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    OUTCOME_TRY(connection->write(MessageManager::lsMsg()));
    return Status::LS_SENT;
  }

  outcome::result<Multiselect::Status> Multiselect::onUnexpectedRequestResponse(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) const {
    log_->info("got a unexpected request-response combination");
    return Status::NEGOTIATION_FAIL;
  }

  outcome::result<Multiselect::Status> Multiselect::onGarbagedStreamStatus(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) const {
    log_->error("there is some garbage in stream state status");
    return Status::NEGOTIATION_FAIL;
  }

}  // namespace libp2p::protocol_muxer
