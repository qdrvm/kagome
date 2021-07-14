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

  outcome::result<ParachainHost::DutyRoster> ParachainHostImpl::duty_roster() {
    return executor_->callAtLatest<DutyRoster>("ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParachainHost::ParachainId>>
  ParachainHostImpl::active_parachains() {
    return executor_->callAtLatest<std::vector<ParachainId>>(
        "ParachainHost_active_parachains");
  }

  outcome::result<boost::optional<common::Buffer>>
  ParachainHostImpl::parachain_head(ParachainId id) {
    return executor_->callAtLatest<boost::optional<Buffer>>(
        "ParachainHost_parachain_head", id);
  }

  outcome::result<boost::optional<common::Buffer>>
  ParachainHostImpl::parachain_code(ParachainId id) {
    return executor_->callAtLatest<boost::optional<common::Buffer>>(
        "ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ParachainHost::ValidatorId>>
  ParachainHostImpl::validators() {
    return executor_->callAtLatest<std::vector<ValidatorId>>(
        "ParachainHost_validators");
  }

}  // namespace kagome::runtime
