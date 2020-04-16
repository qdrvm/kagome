/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_ROUTER_LIBP2P_HPP
#define KAGOME_ROUTER_LIBP2P_HPP

#include <memory>

#include "common/logger.hpp"
#include "consensus/grandpa/round_observer.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/babe_observer.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/router.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/gossip_message.hpp"

namespace kagome::network {
  class RouterLibp2p : public Router,
                       public std::enable_shared_from_this<RouterLibp2p> {
    using Stream = libp2p::connection::Stream;

   public:
    RouterLibp2p(
        libp2p::Host &host,
        std::shared_ptr<BabeObserver> babe_observer,
        std::shared_ptr<consensus::grandpa::RoundObserver> grandpa_observer,
        std::shared_ptr<SyncProtocolObserver> sync_observer);

    ~RouterLibp2p() override = default;

    void init() override;

   private:
    /**
     * Handle stream, which is opened over a Sync protocol
     * @param stream to be handled
     */
    void handleSyncProtocol(const std::shared_ptr<Stream> &stream) const;

    /**
     * Handle stream, which is opened over a Gossip protocol
     * @param stream to be handled
     */
    void handleGossipProtocol(std::shared_ptr<Stream> stream) const;
    void readGossipMessage(std::shared_ptr<Stream> stream) const;

    /**
     * Process a received gossip message
     */
    bool processGossipMessage(const GossipMessage &msg) const;

    libp2p::Host &host_;
    std::shared_ptr<BabeObserver> babe_observer_;
    std::shared_ptr<consensus::grandpa::RoundObserver> grandpa_observer_;
    std::shared_ptr<SyncProtocolObserver> sync_observer_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_ROUTER_LIBP2P_HPP
