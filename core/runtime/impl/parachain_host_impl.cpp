/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;

  ParachainHostImpl::ParachainHostImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      std::shared_ptr<extensions::Extension> extension)
      : RuntimeApi(wasm_provider->getStateCode(), std::move(extension)) {}

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster() {
    return execute<DutyRoster>("ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::active_parachains() {
    return execute<std::vector<ParaId>>("ParachainHost_active_parachains");
  }

  outcome::result<boost::optional<Buffer>> ParachainHostImpl::parachain_head(
      ParachainId id) {
    return execute<boost::optional<Buffer>>("ParachainHost_parachain_head", id);
  }

  outcome::result<boost::optional<Buffer>> ParachainHostImpl::parachain_code(
      ParachainId id) {
    return execute<boost::optional<Buffer>>("ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    return execute<std::vector<ValidatorId>>("ParachainHost_validators");
  }

}  // namespace kagome::runtime
