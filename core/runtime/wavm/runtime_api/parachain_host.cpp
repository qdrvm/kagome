/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/parachain_host.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmParachainHost::WavmParachainHost(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<DutyRoster> WavmParachainHost::duty_roster() override {
    return executor_->callAtLatest<DutyRoster>("ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParachainId>>
  WavmParachainHost::active_parachains() override {
    return executor_->callAtLatest<std::vector<ParachainId>>(
        "ParachainHost_active_parachains");
  }

  outcome::result<boost::optional<Buffer>> WavmParachainHost::parachain_head(
      ParachainId id) override {
    return executor_->callAtLatest<boost::optional<Buffer>>(
        "ParachainHost_parachain_head", id);
  }

  outcome::result<boost::optional<kagome::common::Buffer>>
  WavmParachainHost::parachain_code(ParachainId id) override {
    return executor_->callAtLatest<boost::optional<kagome::common::Buffer>>(
        "ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ValidatorId>> WavmParachainHost::validators()
      override {
    return executor_->callAtLatest<std::vector<ValidatorId>>(
        "ParachainHost_validators");
  }

}  // namespace kagome::runtime::wavm
