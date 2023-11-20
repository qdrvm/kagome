/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/babe_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  BabeApiImpl::BabeApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<consensus::babe::BabeConfiguration>
  BabeApiImpl::configuration(const primitives::BlockHash &block) {
    return executor_->callAt<consensus::babe::BabeConfiguration>(
        block, "BabeApi_configuration");
  }

  outcome::result<consensus::babe::Epoch> BabeApiImpl::next_epoch(
      const primitives::BlockHash &block) {
    return executor_->callAt<consensus::babe::Epoch>(block,
                                                     "BabeApi_next_epoch");
  }
}  // namespace kagome::runtime
