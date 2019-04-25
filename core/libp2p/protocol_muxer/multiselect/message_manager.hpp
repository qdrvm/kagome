/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_MANAGER_HPP
#define KAGOME_MESSAGE_MANAGER_HPP

#include <memory>
#include <optional>
#include <vector>

#include <gsl/span>
#include "common/buffer.hpp"
#include "libp2p/multi/multistream.hpp"
#include "libp2p/multi/uvarint.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Creates and parses Multiselect messages to be sent over the network
   */
  class MessageManager {
   public:
    struct MultiselectMessage {
      enum class MessageType { OPENING, PROTOCOL, PROTOCOLS, LS, NA };

      /// type of the message
      MessageType type_;
      /// zero or more protocols in that message
      std::vector<multi::Multistream> protocols_;
      /// peer, with which we are negotiating
      peer::PeerId peer_id_;
    };

    /**
     * Parse bytes into the message
     * @param bytes to be parsed
     * @return message, if parsing is successful, none otherwise
     */
    std::optional<MultiselectMessage> parseMessage(
        const kagome::common::Buffer &bytes) const;

    /**
     * Create an opening message
     * @param peer_id to be put into the message
     * @return created message
     */
    kagome::common::Buffer openingMsg(const peer::PeerId &peer_id) const;

    /**
     * Create a message with an ls command
     * @return created message
     */
    kagome::common::Buffer lsMsg() const;

    /**
     * Create a message telling the protocol is not supported
     * @return created message
     */
    kagome::common::Buffer naMsg() const;

    /**
     * Create a response message with a single protocol
     * @param peer_id to be put into the message
     * @param protocol to be sent
     * @return created message
     */
    kagome::common::Buffer protocolMsg(
        const peer::PeerId &peer_id, const multi::Multistream &protocol) const;

    /**
     * Create a response message with a list of protocols
     * @param peer_id to be put into the message
     * @param protocols to be sent
     * @return created message
     */
    kagome::common::Buffer protocolsMsg(
        const peer::PeerId &peer_id,
        gsl::span<const multi::Multistream> protocols) const;

   private:
    static constexpr std::string_view kMultiselectHeaderString =
        "/multistream-select/0.3.0";

    static constexpr std::string_view kLsString = "ls\n";
    static constexpr std::string_view kNaString = "na\n";

    const kagome::common::Buffer ls_msg_ =
        kagome::common::Buffer{}
            .put(multi::UVarint{kLsString.size()}.toBytes())
            .put(kLsString);
    const kagome::common::Buffer na_msg_ =
        kagome::common::Buffer{}
            .put(multi::UVarint{kNaString.size()}.toBytes())
            .put(kNaString);
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_MESSAGE_MANAGER_HPP
