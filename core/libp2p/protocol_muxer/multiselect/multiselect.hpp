/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_IMPL_HPP
#define KAGOME_MULTISELECT_IMPL_HPP

#include <memory>
#include <queue>
#include <string_view>
#include <vector>

#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"
#include "libp2p/protocol_muxer/multiselect/message_reader.hpp"
#include "libp2p/protocol_muxer/multiselect/message_writer.hpp"
#include "libp2p/protocol_muxer/multiselect/multiselect_error.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Implementation of a protocol muxer. Read more
   * https://github.com/multiformats/multistream-select
   */
  class Multiselect : public ProtocolMuxer,
                      public std::enable_shared_from_this<Multiselect> {
    friend MessageWriter;
    friend MessageReader;

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

    void selectOneOf(gsl::span<peer::Protocol> protocols,
                     std::shared_ptr<basic::ReadWriter> connection,
                     bool is_initiator, ProtocolHandlerFunc cb) override;

   private:
    /**
     * Negotiate about a protocol
     * @param connection to be negotiated over
     * @param round, about which protocol the negotiation is to take place
     * @return chosen protocol in case of success, error otherwise
     */
    void negotiate(const std::shared_ptr<basic::ReadWriter> &connection,
                   gsl::span<peer::Protocol> protocols, bool is_initiator,
                   ProtocolHandlerFunc handler);

    /**
     * Triggered, when error happens during the negotiation round
     * @param connection_state - state of the connection
     * @param ec - error, which happened
     */
    void negotiationRoundFailed(
        const std::shared_ptr<ConnectionState> &connection_state,
        const std::error_code &ec);

    void onWriteCompleted(
        std::shared_ptr<ConnectionState> connection_state) const;

    void onWriteAckCompleted(
        const std::shared_ptr<ConnectionState> &connection_state,
        const peer::Protocol &protocol);

    void onReadCompleted(std::shared_ptr<ConnectionState> connection_state,
                         MessageManager::MultiselectMessage msg);

    void handleOpeningMsg(
        std::shared_ptr<ConnectionState> connection_state) const;

    void handleProtocolMsg(
        const peer::Protocol &protocol,
        const std::shared_ptr<ConnectionState> &connection_state);

    void handleProtocolsMsg(
        const std::vector<peer::Protocol> &protocols,
        const std::shared_ptr<ConnectionState> &connection_state);

    void onProtocolAfterOpeningOrLs(
        std::shared_ptr<ConnectionState> connection_state,
        const peer::Protocol &protocol);

    void onProtocolsAfterLs(
        const std::shared_ptr<ConnectionState> &connection_state,
        gsl::span<const peer::Protocol> received_protocols);

    void handleLsMsg(const std::shared_ptr<ConnectionState> &connection_state);

    void handleNaMsg(
        const std::shared_ptr<ConnectionState> &connection_state) const;

    void onUnexpectedRequestResponse(
        const std::shared_ptr<ConnectionState> &connection_state) const;

    void onGarbagedStreamStatus(
        const std::shared_ptr<ConnectionState> &connection_state) const;

    void negotiationRoundFinished(
        const std::shared_ptr<ConnectionState> &connection_state,
        const peer::Protocol &chosen_protocol);

    std::tuple<std::shared_ptr<kagome::common::Buffer>,
               std::shared_ptr<boost::asio::streambuf>, size_t>
    getBuffers();

    void clearResources(
        const std::shared_ptr<ConnectionState> &connection_state);

    std::vector<std::shared_ptr<kagome::common::Buffer>> write_buffers_;
    std::vector<std::shared_ptr<boost::asio::streambuf>> read_buffers_;
    std::queue<size_t> free_buffers_;

    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_MULTISELECT_IMPL_HPP
