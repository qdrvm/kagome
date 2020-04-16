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

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_PARACHAIN_HOST_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_PARACHAIN_HOST_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/parachain_host.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime::binaryen {

  class ParachainHostImpl : public RuntimeApi, public ParachainHost {
   public:
    /**
     * @brief constructor
     * @param state_code error or result code
     * @param extension extension instance
     * @param codec scale codec instance
     */
    ParachainHostImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::ExtensionFactory> &extension_factory);

    ~ParachainHostImpl() override = default;

    outcome::result<DutyRoster> duty_roster() override;

    outcome::result<std::vector<ParachainId>> active_parachains() override;

    outcome::result<boost::optional<Buffer>> parachain_head(
        ParachainId id) override;

    outcome::result<boost::optional<Buffer>> parachain_code(
        ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators() override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_PARACHAIN_HOST_IMPL_HPP
