/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authority_discovery/query/query.hpp"

#include "application/app_state_manager.hpp"
#include "authority_discovery/interval.hpp"
#include "authority_discovery/timestamp.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/key_store.hpp"
#include "crypto/sr25519_provider.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "runtime/runtime_api/authority_discovery_api.hpp"

#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/impl/validator_default.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <mutex>
#include <random>

namespace kagome::authority_discovery {
  class QueryImpl : public Query,
                    public libp2p::protocol::kademlia::Validator,
                    public std::enable_shared_from_this<QueryImpl> {
   public:
    enum class Error {
      DECODE_ERROR = 1,
      NO_ADDRESSES,
      INCONSISTENT_PEER_ID,
      INVALID_SIGNATURE,
      KADEMLIA_OUTDATED_VALUE,
    };

    QueryImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
        std::shared_ptr<crypto::KeyStore> key_store,
        std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
        std::shared_ptr<libp2p::crypto::CryptoProvider> libp2p_crypto_provider,
        std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller>
            key_marshaller,
        libp2p::Host &host,
        LazySPtr<libp2p::protocol::kademlia::Kademlia> kademlia,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    bool start();

    std::optional<libp2p::peer::PeerInfo> get(
        const primitives::AuthorityDiscoveryId &authority) const override;

    std::optional<primitives::AuthorityDiscoveryId> get(
        const libp2p::peer::PeerId &peer_id) const override;

    // interface: Validator
    outcome::result<void> validate(
        const libp2p::protocol::kademlia::Key &,
        const libp2p::protocol::kademlia::Value &) override;
    outcome::result<size_t> select(
        const libp2p::protocol::kademlia::Key &,
        const std::vector<libp2p::protocol::kademlia::Value> &) override;

    outcome::result<void> update();
    outcome::result<void> merge(std::optional<std::unordered_set<primitives::AuthorityDiscoveryId>> &&authorities) override;

   private:
    struct Authority {
      Buffer raw;
      std::optional<Timestamp> time;
      libp2p::peer::PeerInfo peer;
    };

    std::optional<primitives::AuthorityDiscoveryId> hashToAuth(
        BufferView key) const;
    void pop();
    outcome::result<void> add(const primitives::AuthorityDiscoveryId &authority,
                              outcome::result<std::vector<uint8_t>> _res);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api_;
    std::shared_ptr<crypto::KeyStore> key_store_;
    std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider_;
    std::shared_ptr<libp2p::crypto::CryptoProvider> libp2p_crypto_provider_;
    std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller_;
    libp2p::Host &host_;
    LazySPtr<libp2p::protocol::kademlia::Kademlia> kademlia_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    ExpIncInterval interval_;

    mutable std::mutex mutex_;
    std::default_random_engine random_;
    libp2p::protocol::kademlia::ValidatorDefault kademlia_validator_;
    std::unordered_map<Hash256, primitives::AuthorityDiscoveryId> hash_to_auth_;
    std::unordered_map<primitives::AuthorityDiscoveryId, Authority>
        auth_to_peer_cache_;
    std::unordered_map<libp2p::peer::PeerId, primitives::AuthorityDiscoveryId>
        peer_to_auth_cache_;
    std::vector<primitives::AuthorityDiscoveryId> queue_;
    size_t active_ = 0;
    std::unordered_set<primitives::AuthorityDiscoveryId> authorities_;
    
    log::Logger log_;
  };
}  // namespace kagome::authority_discovery

OUTCOME_HPP_DECLARE_ERROR(kagome::authority_discovery, QueryImpl::Error)
