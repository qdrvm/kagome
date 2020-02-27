/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/babe_api_impl.hpp"

namespace kagome::runtime::binaryen {

  BabeApiImpl::BabeApiImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::ExtensionFactory> &extension_factory)
      : RuntimeApi(wasm_provider, extension_factory) {}

  outcome::result<primitives::BabeConfiguration> BabeApiImpl::configuration(
      const primitives::BlockId &block_id) {
    return execute<primitives::BabeConfiguration>("BabeApi_configuration",
                                                  block_id);
  }

}  // namespace kagome::runtime::binaryen
