/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/grandpa_impl.hpp"

#include "runtime/impl/runtime_api.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;
  using primitives::WeightedAuthority;

  GrandpaImpl::GrandpaImpl(common::Buffer state_code,
                           std::shared_ptr<extensions::Extension> extension) {
    runtime_ = std::make_unique<RuntimeApi>(std::move(state_code),
                                            std::move(extension));
  }

  GrandpaImpl::~GrandpaImpl() {}

  outcome::result<std::optional<ScheduledChange>> GrandpaImpl::pending_change(
      const Digest &digest) {
    return runtime_->execute<std::optional<ScheduledChange>>(
        "GrandpaApi_grandpa_pending_change", digest);
  }

  outcome::result<std::optional<ForcedChange>> GrandpaImpl::forced_change(
      const Digest &digest) {
    return runtime_->execute<std::optional<ForcedChange>>(
        "GrandpaApi_grandpa_forced_change", digest);
  }

  outcome::result<std::vector<WeightedAuthority>> GrandpaImpl::authorities() {
    return runtime_->execute<std::vector<WeightedAuthority>>(
        "GrandpaApi_grandpa_authorities");
  }
}  // namespace kagome::runtime
