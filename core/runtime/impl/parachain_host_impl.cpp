/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;

  ParachainHostImpl::ParachainHostImpl(
      common::Buffer state_code,  // find out what is it
      std::shared_ptr<extensions::Extension> extension) {
    runtime_ = std::make_unique<RuntimeApi>(std::move(state_code),
                                            std::move(extension));
  }

  ParachainHostImpl::~ParachainHostImpl() {}

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster() {
    return runtime_->execute<DutyRoster>("ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::active_parachains() {
    return runtime_->execute<std::vector<ParaId>>(
        "ParachainHost_active_parachains");
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachain_head(
      ParachainId id) {
    return runtime_->execute<std::optional<Buffer>>(
        "ParachainHost_parachain_head", id);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainCode(
      ParachainId id) {
    return runtime_->execute<std::optional<Buffer>>(
        "ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    return runtime_->execute<std::vector<ValidatorId>>(
        "ParachainHost_validators");
  }

}  // namespace kagome::runtime
