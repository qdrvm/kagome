/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP
#define CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/core.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime::binaryen {

  class CoreImpl : public RuntimeApi, public Core {
   public:
    CoreImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::ExtensionFactory> &extension_factory);

    ~CoreImpl() override = default;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::BlockData &block_data) override;

    outcome::result<void> initialise_block(
        const kagome::primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override;
  };
}  // namespace kagome::runtime::binaryen

#endif  // CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP
