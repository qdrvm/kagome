/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/parachain_host_impl.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;

  ParachainHostImpl::ParachainHostImpl(
      std::shared_ptr<WasmProvider> wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(std::move(wasm_provider), runtime_manager) {}

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster() {
    return execute<DutyRoster>("ParachainHost_duty_roster",
                               CallPersistency::EPHEMERAL);
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::active_parachains() {
    return execute<std::vector<ParaId>>("ParachainHost_active_parachains",
                                        CallPersistency::EPHEMERAL);
  }

  outcome::result<boost::optional<Buffer>> ParachainHostImpl::parachain_head(
      ParachainId id) {
    return execute<boost::optional<Buffer>>(
        "ParachainHost_parachain_head", CallPersistency::EPHEMERAL, id);
  }

  outcome::result<boost::optional<Buffer>> ParachainHostImpl::parachain_code(
      ParachainId id) {
    return execute<boost::optional<Buffer>>(
        "ParachainHost_parachain_code", CallPersistency::EPHEMERAL, id);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    return execute<std::vector<ValidatorId>>("ParachainHost_validators",
                                             CallPersistency::EPHEMERAL);
  }

}  // namespace kagome::runtime::binaryen
