/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_factory_impl.hpp"

#include <WAVM/IR/Module.h>
#include <WAVM/LLVMJIT/LLVMJIT.h>
#include <WAVM/Runtime/Runtime.h>
#include <WAVM/WASM/WASM.h>
#include <libp2p/common/final_action.hpp>

#include "common/buffer.hpp"
#include "common/span_adl.hpp"
#include "crypto/hasher.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_params.hpp"
#include "utils/read_file.hpp"
#include "utils/write_file.hpp"

namespace kagome::runtime::wavm {
  struct Compiled {
    SCALE_TIE(2);

    Buffer wasm, compiled;
  };

  static thread_local std::shared_ptr<Compiled> loading;

  struct ObjectCache : WAVM::Runtime::ObjectCacheInterface {
    std::vector<WAVM::U8> getCachedObject(
        const WAVM::U8 *ptr,
        WAVM::Uptr size,
        std::function<std::vector<WAVM::U8>()> &&get) override {
      std::span input{ptr, size};
      // wasm code was already compiled, other calls are trampolines
      if (loading and SpanAdl{input} == loading->wasm) {
        return loading->compiled;
      }
      return get();
    }
  };

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<ModuleParams> module_params,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<CoreApiFactory> core_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<IntrinsicModule> intrinsic_module,
      std::shared_ptr<crypto::Hasher> hasher)
      : compartment_{std::move(compartment)},
        module_params_{std::move(module_params)},
        host_api_factory_{host_api_factory},
        core_factory_{std::move(core_factory)},
        storage_{storage},
        serializer_{serializer},
        intrinsic_module_{std::move(intrinsic_module)},
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(module_params_ != nullptr);
    BOOST_ASSERT(host_api_factory_ != nullptr);
    BOOST_ASSERT(intrinsic_module_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    [[maybe_unused]] static auto init = [] {
      WAVM::Runtime::setGlobalObjectCache(std::make_shared<ObjectCache>());
      return 0;
    }();
  }

  std::optional<std::string_view> ModuleFactoryImpl::compilerType() const {
    return "wavm";
  }

  CompilationOutcome<void> ModuleFactoryImpl::compile(
      std::filesystem::path path_compiled,
      BufferView code,
      const RuntimeContext::ContextParams &config) const {
    WAVM::IR::Module ir;
    WAVM::WASM::LoadError error;
    if (not WAVM::WASM::loadBinaryModule(
            code.data(), code.size(), ir, &error)) {
      return CompilationError{std::move(error.message)};
    }
    auto compiled =
        WAVM::LLVMJIT::compileModule(ir, WAVM::LLVMJIT::getHostTargetSpec());
    auto raw =
        scale::encode(Compiled{code, Buffer{std::move(compiled)}}).value();
    OUTCOME_TRY(writeFileTmp(path_compiled, raw));
    return outcome::success();
  }

  CompilationOutcome<std::shared_ptr<Module>> ModuleFactoryImpl::loadCompiled(
      std::filesystem::path path_compiled,
      const RuntimeContext::ContextParams &config) const {
    Buffer file;
    if (not readFile(file, path_compiled)) {
      return CompilationError{"read file failed"};
    }
    BOOST_OUTCOME_TRY(loading, scale::decode<std::shared_ptr<Compiled>>(file));
    libp2p::common::FinalAction clear = [] { loading.reset(); };
    auto env_factory = std::make_shared<InstanceEnvironmentFactory>(
        storage_, serializer_, host_api_factory_, core_factory_);
    OUTCOME_TRY(module,
                ModuleImpl::compileFrom(compartment_,
                                        *module_params_,
                                        intrinsic_module_,
                                        env_factory,
                                        loading->wasm,
                                        hasher_->blake2b_256(loading->wasm)));
    return module;
  }
}  // namespace kagome::runtime::wavm
