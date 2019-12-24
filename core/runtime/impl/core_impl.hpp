/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_RUNTIME_CORE_IMPL_HPP
#define CORE_RUNTIME_CORE_IMPL_HPP

#include "runtime/core.hpp"
#include "runtime/impl/runtime_api.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {
  class CoreImpl : public RuntimeApi, public Core {
   public:
    CoreImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::ExtensionFactory> &extension_factory);

    ~CoreImpl() override = default;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const kagome::primitives::Block &block) override;

    outcome::result<void> initialise_block(
        const kagome::primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override;
  };
}  // namespace kagome::runtime

#endif  // CORE_RUNTIME_CORE_IMPL_HPP
