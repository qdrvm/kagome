/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/availability/recovery/recovery.hpp"

#include <mutex>
#include <random>
#include <set>
#include <unordered_map>

#include "log/logger.hpp"
#include "metrics/metrics.hpp"

namespace kagome::application {
  class ChainSpec;
}

namespace kagome::authority_discovery {
  class Query;
}

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::crypto {
  class Hasher;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::network {
  class PeerManager;
  class Router;
}  // namespace kagome::network

namespace kagome::parachain {
  class AvailabilityStore;
}

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::parachain {
  class RecoveryImpl : public Recovery,
                       public std::enable_shared_from_this<RecoveryImpl> {
   public:
    RecoveryImpl(std::shared_ptr<application::ChainSpec> chain_spec,
                 std::shared_ptr<crypto::Hasher> hasher,
                 std::shared_ptr<blockchain::BlockTree> block_tree,
                 std::shared_ptr<runtime::ParachainHost> parachain_api,
                 std::shared_ptr<AvailabilityStore> av_store,
                 std::shared_ptr<authority_discovery::Query> query_audi,
                 std::shared_ptr<network::Router> router,
                 std::shared_ptr<network::PeerManager> pm,
                 std::shared_ptr<crypto::SessionKeys> session_keys);

    void recover(const HashedCandidateReceipt &hashed_receipt,
                 SessionIndex session_index,
                 std::optional<GroupIndex> backing_group,
                 std::optional<CoreIndex> core,
                 Cb cb) override;

    void remove(const CandidateHash &candidate) override;

   private:
    using SelfCb = void (RecoveryImpl::*)(const CandidateHash &);

    struct Active {
      storage::trie::RootHash erasure_encoding_root;
      ChunkIndex chunks_total = 0;
      ChunkIndex chunks_required = 0;
      std::vector<Cb> cb;
      std::vector<primitives::AuthorityDiscoveryId> discovery_keys;
      std::vector<ValidatorIndex> validators_of_group;
      std::vector<ValidatorIndex> order;
      std::set<ValidatorIndex> queried;
      bool systematic_chunk_failed = false;
      std::vector<network::ErasureChunk> chunks;
      std::function<CoreIndex(ValidatorIndex)> val2chunk;
      size_t chunks_active = 0;
    };
    using ActiveMap = std::unordered_map<CandidateHash, Active>;
    using Lock = std::unique_lock<std::mutex>;

    // Full from bakers recovery strategy
    void full_from_bakers_recovery_prepare(const CandidateHash &candidate_hash);
    void full_from_bakers_recovery(const CandidateHash &candidate_hash);

    // Systematic recovery strategy
    void systematic_chunks_recovery_prepare(
        const CandidateHash &candidate_hash);
    void systematic_chunks_recovery(const CandidateHash &candidate_hash);

    // Chunk recovery strategy
    void regular_chunks_recovery_prepare(const CandidateHash &candidate_hash);
    void regular_chunks_recovery(const CandidateHash &candidate_hash);

    // Fetch available data protocol communication
    void send_fetch_available_data_request(const libp2p::PeerId &peer_id,
                                           const CandidateHash &candidate_hash,
                                           SelfCb next_iteration);
    void handle_fetch_available_data_response(
        const libp2p::PeerId &peer_id,
        const CandidateHash &candidate_hash,
        outcome::result<network::FetchAvailableDataResponse> response_res,
        SelfCb next_iteration);

    // Fetch chunk protocol communication
    void send_fetch_chunk_request(const libp2p::PeerId &peer_id,
                                  const CandidateHash &candidate_hash,
                                  ChunkIndex chunk_index,
                                  SelfCb next_iteration);
    void handle_fetch_chunk_response(
        const libp2p::PeerId &peer_id,
        const CandidateHash &candidate_hash,
        outcome::result<network::FetchChunkResponse> response_res,
        SelfCb next_iteration);

    outcome::result<void> check(const Active &active,
                                const AvailableData &data);
    void done(Lock &lock,
              ActiveMap::iterator it,
              const std::optional<outcome::result<AvailableData>> &result);

    log::Logger logger_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<AvailabilityStore> av_store_;
    std::shared_ptr<authority_discovery::Query> query_audi_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;

    std::mutex mutex_;
    std::default_random_engine random_;
    std::unordered_map<CandidateHash, outcome::result<AvailableData>> cached_;
    ActiveMap active_;

    // metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Counter *full_recoveries_started_;
    std::unordered_map<std::string,
                       std::unordered_map<std::string, metrics::Counter *>>
        full_recoveries_finished_;
  };
}  // namespace kagome::parachain
