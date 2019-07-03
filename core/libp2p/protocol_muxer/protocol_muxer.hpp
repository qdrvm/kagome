/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MUXER_HPP
#define KAGOME_PROTOCOL_MUXER_HPP

#include <memory>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/basic/readwriter.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about the
   * protocols, which are going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
    /**
     * Select a protocol for a given connection
     * @param protocols - set of protocols, one of which should be chosen during
     * the negotiation
     * @param connection, for which the protocol is being chosen
     * @param is_initiator - true, if we initiated the connection and thus
     * taking lead in the Multiselect protocol; false otherwise
     * @return chosen protocol or error
     */
    virtual void selectOneOf(
        gsl::span<const peer::Protocol> protocols,
        std::shared_ptr<basic::ReadWriter> connection, bool is_initiator,
        std::function<void(outcome::result<peer::Protocol>)> cb) const = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_PROTOCOL_MUXER_HPP
