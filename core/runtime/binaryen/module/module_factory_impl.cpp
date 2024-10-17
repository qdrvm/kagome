/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_factory_impl.hpp"

#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_memory_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/core_api_factory_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "utils/read_file.hpp"
#include "utils/write_file.hpp"

namespace kagome::runtime::binaryen {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : env_factory_{std::move(env_factory)},
        storage_{std::move(storage)},
        hasher_(std::move(hasher)) {}

  std::optional<std::string_view> ModuleFactoryImpl::compilerType() const {
    return std::nullopt;
  }

  CompilationOutcome<void> ModuleFactoryImpl::compile(
      std::filesystem::path path_compiled,
      BufferView code,
      const RuntimeContext::ContextParams &config) const {
    if (config.wasm_ext_bulk_memory) {
      return CompilationError{"bulk memory is not supported"};
    }
    OUTCOME_TRY(writeFileTmp(path_compiled, code));
    return outcome::success();
  }

  CompilationOutcome<std::shared_ptr<Module>> ModuleFactoryImpl::loadCompiled(
      const kagome::parachain::PvfWorkerInputCodeParams &code_params) const {
    Buffer code;
    if (not readFile(code, code_params.path)) {
      return CompilationError{"read file failed"};
    }
    if (code_params.context_params.wasm_ext_bulk_memory) {
      return CompilationError{"bulk memory is not supported"};
    }
    OUTCOME_TRY(module,
                ModuleImpl::createFromCode(
                    code, env_factory_, hasher_->blake2b_256(code)));
    return module;
  }
}  // namespace kagome::runtime::binaryen
