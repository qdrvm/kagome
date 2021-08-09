/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  ParachainHostImpl::ParachainHostImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<ParachainHost::DutyRoster> ParachainHostImpl::duty_roster(
      const primitives::BlockHash &block) {
    return executor_->callAt<DutyRoster>(block, "ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParachainHost::ParachainId>>
  ParachainHostImpl::active_parachains(const primitives::BlockHash &block) {
    return executor_->callAt<std::vector<ParachainId>>(
        block, "ParachainHost_active_parachains");
  }

  outcome::result<boost::optional<common::Buffer>>
  ParachainHostImpl::parachain_head(const primitives::BlockHash &block,
                                    ParachainId id) {
    return executor_->callAt<boost::optional<Buffer>>(
        block, "ParachainHost_parachain_head", id);
  }

  outcome::result<boost::optional<common::Buffer>>
  ParachainHostImpl::parachain_code(const primitives::BlockHash &block,
                                    ParachainId id) {
    return executor_->callAt<boost::optional<common::Buffer>>(
        block, "ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ParachainHost::ValidatorId>>
  ParachainHostImpl::validators(const primitives::BlockHash &block) {
    return executor_->callAt<std::vector<ValidatorId>>(
        block, "ParachainHost_validators");
  }

}  // namespace kagome::runtime
