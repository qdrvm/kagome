/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_GRANDPAAPI
#define KAGOME_RUNTIME_WAVM_GRANDPAAPI

#include "runtime/grandpa_api.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmGrandpaApi final: public GrandpaApi {
   public:
    WavmGrandpaApi(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<boost::optional<ScheduledChange>> pending_change(
        const Digest &digest) override {
      return executor_->call<boost::optional<ScheduledChange>>(
          "GrandpaApi_pending_change", digest);
    }

    outcome::result<boost::optional<ForcedChange>> forced_change(
        const Digest &digest) override {
      return executor_->call<boost::optional<ForcedChange>>(
          "GrandpaApi_forced_change", digest);
    }

    outcome::result<AuthorityList> authorities(
        const primitives::BlockId &block_id) override {
      return executor_->call<AuthorityList>(
          "GrandpaApi_authorities", block_id);
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_RUNTIME_WAVM_GRANDPAAPI
