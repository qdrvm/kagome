/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include "parachain/availability/store/store.hpp"
#include "parachain/validator/i_parachain_processor.hpp"

namespace kagome::parachain {

  class ParachainStorageImpl : public ParachainStorage {
   protected:
    std::shared_ptr<parachain::AvailabilityStore> av_store_;

   public:
    ParachainStorageImpl(
        std::shared_ptr<parachain::AvailabilityStore> av_store);

    outcome::result<network::FetchChunkResponse> OnFetchChunkRequest(
        const network::FetchChunkRequest &request) override;

    outcome::result<network::FetchChunkResponseObsolete>
    OnFetchChunkRequestObsolete(
        const network::FetchChunkRequest &request) override;

    network::ResponsePov getPov(CandidateHash &&candidate_hash) override;
  };

}  // namespace kagome::parachain
