/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/availability/fetch/fetch.hpp"

#include <mutex>
#include <unordered_map>

#include "authority_discovery/query/query.hpp"
#include "network/router.hpp"
#include "parachain/availability/store/store.hpp"

namespace kagome::parachain {
  class FetchImpl : public Fetch,
                    public std::enable_shared_from_this<FetchImpl> {
   public:
    FetchImpl(std::shared_ptr<AvailabilityStore> av_store,
              std::shared_ptr<authority_discovery::Query> query_audi,
              std::shared_ptr<network::Router> router);

    void fetch(ValidatorIndex chunk_index,
               const runtime::OccupiedCore &core,
               const runtime::SessionInfo &session) override;

   private:
    struct Active {
      ValidatorIndex chunk_index;
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

    std::mutex mutex_;
    std::unordered_map<CandidateHash, Active> active_;
  };
}  // namespace kagome::parachain
