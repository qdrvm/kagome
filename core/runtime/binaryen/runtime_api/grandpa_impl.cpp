/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
