/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
#define KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP

#include <memory>

#include "api/service/author/impl/author_api_observer_impl.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/round_observer.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/babe_observer.hpp"
#include "network/gossiper.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/router.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/gossip_message.hpp"

namespace kagome::network {
  class RouterLibp2p : public Router,
                       public std::enable_shared_from_this<RouterLibp2p> {
   public:
    RouterLibp2p(
        libp2p::Host &host,
        std::shared_ptr<BabeObserver> babe_observer,
        std::shared_ptr<consensus::grandpa::RoundObserver> grandpa_observer,
        std::shared_ptr<SyncProtocolObserver> sync_observer,
        std::shared_ptr<api::AuthorApiObserver> author_api_observer,
        std::shared_ptr<Gossiper> gossiper);

    ~RouterLibp2p() override = default;

    void init() override;

    void handleSyncProtocol(
        const std::shared_ptr<Stream> &stream) const override;

    void handleGossipProtocol(std::shared_ptr<Stream> stream) const override;

   private:
    void readGossipMessage(std::shared_ptr<Stream> stream) const;

    /**
     * Process a received gossip message
     */
    bool processGossipMessage(const GossipMessage &msg) const;

    libp2p::Host &host_;
    std::shared_ptr<BabeObserver> babe_observer_;
    std::shared_ptr<consensus::grandpa::RoundObserver> grandpa_observer_;
    std::shared_ptr<SyncProtocolObserver> sync_observer_;
    std::shared_ptr<api::AuthorApiObserver> author_api_observer_;
    std::shared_ptr<Gossiper> gossiper_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
