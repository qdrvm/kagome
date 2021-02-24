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
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory)
      : RuntimeApi(runtime_env_factory) {}

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster() {
    return execute<DutyRoster>("ParachainHost_duty_roster",
        CallConfig{.persistency = CallPersistency::EPHEMERAL});
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::active_parachains() {
    return execute<std::vector<ParaId>>(
        "ParachainHost_active_parachains",
        CallConfig{.persistency = CallPersistency::EPHEMERAL});
  }

  outcome::result<boost::optional<Buffer>> ParachainHostImpl::parachain_head(
      ParachainId id) {
    return execute<boost::optional<Buffer>>(
        "ParachainHost_parachain_head",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        id);
  }

  outcome::result<boost::optional<Buffer>> ParachainHostImpl::parachain_code(
      ParachainId id) {
    return execute<boost::optional<Buffer>>(
        "ParachainHost_parachain_code",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        id);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    return execute<std::vector<ValidatorId>>(
        "ParachainHost_validators",
        CallConfig{.persistency = CallPersistency::EPHEMERAL});
  }

}  // namespace kagome::runtime::binaryen
