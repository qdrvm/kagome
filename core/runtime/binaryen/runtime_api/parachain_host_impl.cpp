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

#include "runtime/binaryen/runtime_api/parachain_host_impl.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;

  ParachainHostImpl::ParachainHostImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

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
