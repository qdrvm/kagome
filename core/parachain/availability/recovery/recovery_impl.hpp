/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/availability/recovery/recovery.hpp"

#include <mutex>
#include <random>
#include <unordered_map>

#include "authority_discovery/query/query.hpp"
#include "blockchain/block_tree.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/router.hpp"
#include "parachain/availability/store/store.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::application {
  class ChainSpec;
}

namespace kagome::network {
  class PeerManager;
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
                 std::shared_ptr<network::PeerManager> pm);

    void recover(const HashedCandidateReceipt &hashed_receipt,
                 SessionIndex session_index,
                 std::optional<GroupIndex> backing_group,
                 Cb cb) override;

    void remove(const CandidateHash &candidate) override;

   private:
    struct Active {
      storage::trie::RootHash erasure_encoding_root;
      size_t chunks_required = 0;
      std::vector<Cb> cb;
      std::vector<primitives::AuthorityDiscoveryId> validators;
      std::vector<ValidatorIndex> order;
      std::vector<network::ErasureChunk> chunks;
      size_t chunks_active = 0;
    };
    using ActiveMap = std::unordered_map<CandidateHash, Active>;
    using Lock = std::unique_lock<std::mutex>;

    void back(const CandidateHash &candidate_hash);
    void back(const CandidateHash &candidate_hash,
              outcome::result<network::FetchAvailableDataResponse> _backed);
    void chunk(const CandidateHash &candidate_hash);
    void chunk(const CandidateHash &candidate_hash,
               ChunkIndex index,
               outcome::result<network::FetchChunkResponse> _chunk);
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
