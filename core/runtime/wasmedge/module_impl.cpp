/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/module_impl.hpp"

#include <filesystem>
#include <fstream>
#include <memory>

#include <wasmedge.h>

#include "common/mp_utils.hpp"
#include "crypto/sha/sha256.hpp"
#include "runtime/wasmedge/instance_environment_factory.hpp"
#include "runtime/wasmedge/memory_provider.hpp"
#include "runtime/wasmedge/module_instance_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

namespace kagome::runtime::wasmedge {

  ModuleImpl::ModuleImpl(
      const WasmEdge_ASTModuleContext *ast_ctx,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory)
      : env_factory_{std::move(env_factory)}, ast_ctx_{ast_ctx} {
    BOOST_ASSERT(env_factory_ != nullptr);
    BOOST_ASSERT(ast_ctx_ != nullptr);
    // vm_ = WasmEdge_VMCreate(NULL, NULL);
    // WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("env");
    // auto Res = WasmEdge_VMRegisterModuleFromASTModule(vm_, ModuleName,
    // ast_ctx_); WasmEdge_StringDelete(ModuleName); BOOST_ASSERT(vm_ !=
    // nullptr);
  }

  outcome::result<std::unique_ptr<ModuleImpl>> ModuleImpl::createFromCode(
      const std::vector<uint8_t> &code,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory) {
    auto log = log::createLogger("wasm_module", "wasmedge");

    auto ConfCtx = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureCompilerSetOptimizationLevel(
        ConfCtx, WasmEdge_CompilerOptimizationLevel_O3);
    auto CompilerCtx = WasmEdge_CompilerCreate(ConfCtx);

    auto hash = crypto::sha256(code);
    std::string source = "source-" + hash.toHex() + ".wasm";
    std::string result = "result-" + hash.toHex() + ".wasm.so";

    if (not std::filesystem::exists(source)) {
      std::ofstream ofs;
      ofs.open(source);
      ofs.write(reinterpret_cast<const char *>(code.data()), code.size());
      ofs.close();
    }

    if (not std::filesystem::exists(result)) {
      WasmEdge_CompilerCompile(CompilerCtx, source.c_str(), result.c_str());
    }

    auto *LoaderCtx = WasmEdge_LoaderCreate(ConfCtx);
    WasmEdge_ASTModuleContext *ASTCtx = nullptr;
    auto Res = WasmEdge_LoaderParseFromFile(LoaderCtx, &ASTCtx, result.c_str());

    std::unique_ptr<ModuleImpl> wasm_module_impl(
        new ModuleImpl(ASTCtx, std::move(env_factory)));
    return wasm_module_impl;
  }

  outcome::result<std::shared_ptr<ModuleInstance>> ModuleImpl::instantiate()
      const {
    auto env = env_factory_->make();
    return std::make_shared<ModuleInstanceImpl>(std::move(env.env), this);
  }

}  // namespace kagome::runtime::wasmedge
