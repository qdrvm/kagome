/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_GRANDPAAPI
#define KAGOME_RUNTIME_WAVM_GRANDPAAPI

#include "runtime/grandpa_api.hpp"

#include "blockchain/block_header_repository.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmGrandpaApi final : public GrandpaApi {
   public:
    WavmGrandpaApi(std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
                   std::shared_ptr<Executor> executor)
    : block_header_repo_{std::move(block_header_repo)},
      executor_{std::move(executor)} {
      BOOST_ASSERT(block_header_repo_);
      BOOST_ASSERT(executor_);
    }

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override {
      return executor_->callAtLatest<boost::optional<ScheduledChange>>(
          "GrandpaApi_pending_change", digest);
    }

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override {
      return executor_->callAtLatest<boost::optional<ForcedChange>>(
          "GrandpaApi_forced_change", digest);
    }

    outcome::result<AuthorityList> authorities(
        const primitives::BlockId &block_id) override {
      OUTCOME_TRY(header, block_header_repo_->getBlockHeader(block_id));
      return executor_->callAt<AuthorityList>(
          header.state_root, "GrandpaApi_grandpa_authorities");
    }

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_RUNTIME_WAVM_GRANDPAAPI
