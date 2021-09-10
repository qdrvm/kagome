/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/module_impl.hpp"

#include <memory>

#include <wasmedge.h>

#include "common/mp_utils.hpp"
#include "runtime/wasmedge/instance_environment_factory.hpp"
#include "runtime/wasmedge/memory_provider.hpp"
#include "runtime/wasmedge/module_instance_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

namespace kagome::runtime::wasmedge {

  ModuleImpl::ModuleImpl(
      WasmEdge_ASTModuleContext *ast_ctx,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory)
      : env_factory_{std::move(env_factory)}, ast_ctx_{ast_ctx} {
    BOOST_ASSERT(env_factory_ != nullptr);
    BOOST_ASSERT(ast_ctx_ != nullptr);
  }

  outcome::result<std::unique_ptr<ModuleImpl>> ModuleImpl::createFromCode(
      const std::vector<uint8_t> &code,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory) {
    auto log = log::createLogger("wasm_module", "wasmedge");

    auto *LoaderCtx = WasmEdge_LoaderCreate(nullptr);
    WasmEdge_ASTModuleContext *ASTCtx = nullptr;
    auto Res = WasmEdge_LoaderParseFromBuffer(
        LoaderCtx, &ASTCtx, code.data(), code.size());

    std::unique_ptr<ModuleImpl> wasm_module_impl(
        new ModuleImpl(ASTCtx, std::move(env_factory)));
    return wasm_module_impl;
  }

  outcome::result<std::unique_ptr<ModuleInstance>> ModuleImpl::instantiate()
      const {
    auto env = env_factory_->make();
    return std::make_unique<ModuleInstanceImpl>(
        std::move(env.env), shared_from_this(), env.vm);
  }

}  // namespace kagome::runtime::wasmedge
