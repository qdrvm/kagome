/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_COMMUNICATOR_HPP
#define KAGOME_MULTISELECT_COMMUNICATOR_HPP

#include <optional>
#include <vector>

#include <gsl/span>
#include "common/buffer.hpp"
#include "libp2p/multi/multistream.hpp"
#include "libp2p/multi/uvarint.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Creates and parses Multiselect messages to be sent over the network
   */
  class MultiselectCommunicator {
   public:
    struct MultiselectMessage {
      enum class MessageType { OPENING, PROTOCOL, PROTOCOLS, LS, NA };

      /// type of the message
      MessageType type_;
      /// zero or more protocols in that message
      std::vector<multi::Multistream> protocols_;
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
     * @return created message
     */
    kagome::common::Buffer openingMsg() const;

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
     * @param protocol to be sent
     * @return created message
     */
    kagome::common::Buffer protocolMsg(
        const multi::Multistream &protocol) const;

    /**
     * Create a response message with a list of protocols
     * @param protocols to be sent
     * @return created message
     */
    kagome::common::Buffer protocolsMsg(
        gsl::span<const multi::Multistream> protocols) const;

   private:
    static constexpr std::string_view kProtocolHeaderString =
        "/multistream-select/0.3.0\n";
    static constexpr std::string_view kLsString = "ls\n";
    static constexpr std::string_view kNaString = "na\n";

    const kagome::common::Buffer multiselect_header_ =
        kagome::common::Buffer{}
            .put(multi::UVarint{kProtocolHeaderString.size()}.toBytes())
            .put(kProtocolHeaderString);
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

#endif  // KAGOME_MULTISELECT_COMMUNICATOR_HPP
