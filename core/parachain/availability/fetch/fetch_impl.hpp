/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/availability/fetch/fetch.hpp"

#include <mutex>
#include <unordered_map>

namespace kagome::authority_discovery {
  class Query;
}

namespace kagome::network {
  class PeerManager;
  class Router;
}  // namespace kagome::network

namespace kagome::parachain {
  class AvailabilityStore;
}

namespace kagome::parachain {
  class FetchImpl : public Fetch,
                    public std::enable_shared_from_this<FetchImpl> {
   public:
    FetchImpl(std::shared_ptr<AvailabilityStore> av_store,
              std::shared_ptr<authority_discovery::Query> query_audi,
              std::shared_ptr<network::Router> router,
              std::shared_ptr<network::PeerManager> pm);

    void fetch(ChunkIndex chunk_index,
               const runtime::OccupiedCore &core,
               const runtime::SessionInfo &session) override;

   private:
    struct Active {
      ChunkIndex chunk_index;
      RelayHash relay_parent;
      std::vector<primitives::AuthorityDiscoveryId> validators;
      storage::trie::RootHash erasure_encoding_root;
    };

    void fetch(const CandidateHash &candidate_hash);
    void fetch(const CandidateHash &candidate_hash,
               outcome::result<network::FetchChunkResponse> _chunk);

    std::shared_ptr<AvailabilityStore> av_store_;
    std::shared_ptr<authority_discovery::Query> query_audi_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<network::PeerManager> pm_;

    std::mutex mutex_;
    std::unordered_map<CandidateHash, Active> active_;
  };
}  // namespace kagome::parachain
