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

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_BABE_API_IMPL_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_BABE_API_IMPL_HPP

#include "runtime/babe_api.hpp"

#include "extensions/extension.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime::binaryen {

  class BabeApiImpl : public RuntimeApi, public BabeApi {
   public:
    ~BabeApiImpl() override = default;

    BabeApiImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::ExtensionFactory> &extension_factory);

    outcome::result<primitives::BabeConfiguration> configuration() override;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_BABE_API_IMPL_HPP
