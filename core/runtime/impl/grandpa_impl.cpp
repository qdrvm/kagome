/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/grandpa_impl.hpp"

#include "primitives/authority.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::Authority;
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;

  GrandpaImpl::GrandpaImpl(common::Buffer state_code,
                           std::shared_ptr<extensions::Extension> extension)
      : RuntimeApi(std::move(state_code), std::move(extension)) {}

  outcome::result<boost::optional<ScheduledChange>> GrandpaImpl::pending_change(
      const Digest &digest) {
    return execute<boost::optional<ScheduledChange>>(
        "GrandpaApi_grandpa_pending_change", digest);
  }

  outcome::result<boost::optional<ForcedChange>> GrandpaImpl::forced_change(
      const Digest &digest) {
    return execute<boost::optional<ForcedChange>>(
        "GrandpaApi_grandpa_forced_change", digest);
  }

  outcome::result<std::vector<Authority>> GrandpaImpl::authorities() {
    return execute<std::vector<Authority>>("GrandpaApi_grandpa_authorities");
  }
}  // namespace kagome::runtime
