/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_IMPL_HPP
#define KAGOME_MULTISELECT_IMPL_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"
#include "libp2p/protocol_muxer/multiselect/message_reader.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Implementation of a protocol muxer. Read more
   * https://github.com/multiformats/multistream-select
   */
  class Multiselect : public ProtocolMuxer {
   public:
    /**
     * Create a Multiselect instance
     * @param logger to write debug messages to
     */
    explicit Multiselect(kagome::common::Logger logger =
                             kagome::common::createLogger("Multiselect"));

    Multiselect(const Multiselect &other) = delete;
    Multiselect &operator=(const Multiselect &other) = delete;
    Multiselect(Multiselect &&other) = default;
    Multiselect &operator=(Multiselect &&other) = default;

    ~Multiselect() override = default;

    outcome::result<peer::Protocol> selectOneOf(
        gsl::span<const peer::Protocol> supported_protocols,
        std::shared_ptr<basic::ReadWriteCloser> connection,
        bool is_initiator) const override;

    enum class MultiselectError {
      PROTOCOLS_LIST_EMPTY = 1,
      NEGOTIATION_FAILED,
      INTERNAL_ERROR
    };

   private:
    enum class Status {
      NOTHING_SENT,
      OPENING_SENT,
      PROTOCOL_SENT,
      PROTOCOLS_SENT,
      LS_SENT,
      NA_SENT,
      NEGOTIATION_SUCCESS,
      NEGOTIATION_FAIL
    };

    /**
     * Negotiate about a protocol
     * @param connection to be negotiated over
     * @param round, about which protocol the negotiation is to take place
     * @return chosen protocol in case of success, error otherwise
     */
    outcome::result<peer::Protocol> negotiate(
        const std::shared_ptr<basic::ReadWriteCloser> &connection,
        gsl::span<const peer::Protocol> protocols, Status initial_status) const;

    /**
     * Finish a negotiation process
     * @param status, in which the negotiation ended
     * @param protocol, which was (or not) chosen
     * @return chosen protocol in case of success, error otherwise
     */
    outcome::result<peer::Protocol> finalizeNegotiation(
        Status status, const peer::Protocol &protocol) const;

    /**
     * Handle a message, signalizing about start of the negotiation
     * @param connection, over which the message came
     * @param status of the negotiation process
     * @return status after message handling
     */
    outcome::result<Status> handleOpeningMsg(
        const std::shared_ptr<basic::ReadWriteCloser> &connection,
        Status status) const;

    /**
     * Handle a message, containing a protocol
     * @param connection, over which the message came
     * @param protocol, which was in the message
     * @param prev_protocol - protocol, which we sent the last time
     * @param status of the negotiation process
     * @param round, about which protocol the negotiation is held
     * @return status after message handling
     */
    outcome::result<Status> handleProtocolMsg(
        const std::shared_ptr<basic::ReadWriteCloser> &connection,
        gsl::span<const peer::Protocol> supported_protocols,
        const peer::Protocol &protocol, const peer::Protocol &prev_protocol,
        Status status) const;

    /**
     * Handle a message, containing protocols
     * @param connection, over which the message came
     * @param protocols, which were in the message
     * @param status of the negotiation process
     * @param round, about which protocol the negotiation is held
     * @return status after message handling @and chosen protocol (if any)
     */
    outcome::result<std::pair<Status, peer::Protocol>> handleProtocolsMsg(
        const std::shared_ptr<basic::ReadWriteCloser> &connection,
        gsl::span<const peer::Protocol> supported_protocols,
        const std::vector<peer::Protocol> &protocols, Status status) const;

    /**
     * Handle a message, containing an ls
     * @param connection, over which the message came
     * @param round, about which protocol the negotiation is held
     * @return status after message handling
     */
    outcome::result<Status> handleLsMsg(
        const std::shared_ptr<basic::ReadWriteCloser> &connection,
        gsl::span<const peer::Protocol> supported_protocols) const;

    /**
     * Handle a message, containing an na
     * @param connection, over which the message came
     * @return status after message handling
     */
    outcome::result<Status> handleNaMsg(
        const std::shared_ptr<basic::ReadWriteCloser> &connection) const;

    /**
     * Triggered, when an unexpected message arrives to as a response to our
     * request
     * @param connection, over which the message came
     * @return status after handling
     */
    outcome::result<Status> onUnexpectedRequestResponse(
        const std::shared_ptr<basic::ReadWriteCloser> &connection) const;

    /**
     * Triggered, when a stream status contains garbage value
     * @param connection, over which the message came
     * @return status after handling
     */
    outcome::result<Status> onGarbagedStreamStatus(
        const std::shared_ptr<basic::ReadWriteCloser> &connection) const;

    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer, Multiselect::MultiselectError)

#endif  // KAGOME_MULTISELECT_IMPL_HPP
