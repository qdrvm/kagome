/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/beefy.hpp"

#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {
  BeefyApiImpl::BeefyApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::optional<primitives::BlockNumber>> BeefyApiImpl::genesis(
      const primitives::BlockHash &block) {
    auto r = executor_->callAt<std::optional<primitives::BlockNumber>>(
        block, "BeefyApi_beefy_genesis");
    if (r) {
      return std::move(r.value());
    }
    if (r.error() == RuntimeTransactionError::EXPORT_FUNCTION_NOT_FOUND) {
      return std::nullopt;
    }
    return r.error();
  }

  outcome::result<std::optional<consensus::beefy::ValidatorSet>>
  BeefyApiImpl::validatorSet(const primitives::BlockHash &block) {
    return executor_->callAt<std::optional<consensus::beefy::ValidatorSet>>(
        block, "BeefyApi_validator_set");
  }
}  // namespace kagome::runtime
