/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/grandpa_impl.hpp"

#include "primitives/authority.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::Authority;
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;

  GrandpaImpl::GrandpaImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

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

  outcome::result<std::vector<Authority>> GrandpaImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<std::vector<Authority>>("GrandpaApi_grandpa_authorities",
                                           block_id);
  }
}  // namespace kagome::runtime::binaryen
