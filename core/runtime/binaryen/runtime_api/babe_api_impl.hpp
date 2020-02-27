/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
