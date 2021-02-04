/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNC_CLIENTS_SET_HPP
#define KAGOME_SYNC_CLIENTS_SET_HPP

#include <memory>
#include <unordered_set>

#include "libp2p/peer/peer_id.hpp"
#include "network/impl/remote_sync_protocol_client.hpp"
#include "network/sync_protocol_client.hpp"

namespace kagome::network {
  /**
   * Keeps all known Sync clients
   */
  class SyncClientsSet {
   public:
    SyncClientsSet(libp2p::Host &host,
                   std::shared_ptr<application::ChainSpec> chain_spec)
        : host_(host), chain_spec_(std::move(chain_spec)) {
      BOOST_ASSERT(chain_spec_ != nullptr);
    }

    std::shared_ptr<SyncProtocolClient> get(libp2p::peer::PeerId peer_id) {
      auto it = clients_.find(peer_id);
      if (it == clients_.end()) {
        it = clients_
                 .emplace(peer_id,
                          std::make_shared<network::RemoteSyncProtocolClient>(
                              host_, peer_id, chain_spec_))
                 .first;
      }
      return it->second;
    }
    void remove(libp2p::peer::PeerId peer_id) {
      clients_.erase(peer_id);
    }
    const auto& clients() const {
      return clients_;
    }

   private:
    libp2p::Host &host_;
    std::shared_ptr<application::ChainSpec> chain_spec_;

    std::unordered_map<libp2p::peer::PeerId,
                       std::shared_ptr<SyncProtocolClient>>
        clients_;
  };
}  // namespace kagome::network

#endif  // KAGOME_SYNC_CLIENTS_SET_HPP
