/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParachainId;
  using primitives::parachain::ValidatorId;

  outcome::result<DutyRoster> ParachainHostImpl::dutyRoster() {
    return {{}};
  }

  outcome::result<std::vector<ParachainId>>
  ParachainHostImpl::activeParachains() {
    return {{}};
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainHead(
      ParachainId id) {
    return Buffer{};
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainCode(
      ParachainId id) {
    return Buffer{};
  };

  outcome::result<std::vector<ValidatorId>> validators() {
    return {{}};
  };



}  // namespace kagome::runtime
