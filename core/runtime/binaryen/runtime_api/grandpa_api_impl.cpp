/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/grandpa_api_impl.hpp"

#include "primitives/authority.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::Authority;
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;

  GrandpaApiImpl::GrandpaApiImpl(
      const std::shared_ptr<WasmProvider> &wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(wasm_provider, runtime_manager) {}

  outcome::result<boost::optional<ScheduledChange>>
  GrandpaApiImpl::pending_change(const Digest &digest) {
    return execute<boost::optional<ScheduledChange>>(
        "GrandpaApi_grandpa_pending_change",
        CallPersistency::EPHEMERAL,
        digest);
  }

  outcome::result<boost::optional<ForcedChange>> GrandpaApiImpl::forced_change(
      const Digest &digest) {
    return execute<boost::optional<ForcedChange>>(
        "GrandpaApi_grandpa_forced_change", CallPersistency::EPHEMERAL, digest);
  }

  outcome::result<primitives::AuthorityList> GrandpaApiImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<primitives::AuthorityList>(
        "GrandpaApi_grandpa_authorities", CallPersistency::EPHEMERAL);
  }
}  // namespace kagome::runtime::binaryen
