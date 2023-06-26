/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/babe_api.hpp"

#include "runtime/common/executor.hpp"

namespace kagome::runtime {

  BabeApiImpl::BabeApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::BabeConfiguration> BabeApiImpl::configuration(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ref, cache_.get_else(block, [&] {
      return executor_->callAt<primitives::BabeConfiguration>(
          block, "BabeApi_configuration");
    }));
    return ref.get();
  }

}  // namespace kagome::runtime
