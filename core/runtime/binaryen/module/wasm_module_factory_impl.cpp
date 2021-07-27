/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/wasm_module_factory_impl.hpp"

#include "runtime/binaryen/module/wasm_module_impl.hpp"

namespace kagome::runtime::binaryen {

  outcome::result<std::unique_ptr<WasmModule>>
  WasmModuleFactoryImpl::createModule(
      const common::Buffer &code,
      std::shared_ptr<RuntimeExternalInterface> rei,
      std::shared_ptr<TrieStorageProvider> storage_provider) const {
    auto res = WasmModuleImpl::createFromCode(code, rei, storage_provider);
    if (res.has_value()) {
      return std::unique_ptr<WasmModule>(std::move(res.value()));
    }
    return res.error();
  }

}  // namespace kagome::runtime::binaryen
