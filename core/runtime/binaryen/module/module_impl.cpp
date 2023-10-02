/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_impl.hpp"

#include <memory>

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>

#include "common/int_serialization.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/binaryen/module/module_instance_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

using namespace kagome::common::literals;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen, ModuleImpl::Error, e) {
  using Error = kagome::runtime::binaryen::ModuleImpl::Error;
  switch (e) {
    case Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
  }
  return "Unknown error";
}

namespace kagome::runtime::binaryen {

  ModuleImpl::ModuleImpl(
      std::unique_ptr<wasm::Module> &&module,
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
      const common::Hash256 &code_hash)
      : module_factory_{std::move(module_factory)},
        env_factory_{std::move(env_factory)},
        module_{std::move(module)},
        code_hash_(code_hash) {
    BOOST_ASSERT(module_ != nullptr);
    BOOST_ASSERT(env_factory_ != nullptr);
    BOOST_ASSERT(module_factory_ != nullptr);
  }

  outcome::result<std::shared_ptr<ModuleImpl>> ModuleImpl::createFromCode(
      const std::vector<uint8_t> &code,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<const ModuleFactory> module_factory,
      const common::Hash256 &code_hash) {
    auto log = log::createLogger("wasm_module", "binaryen");
    // that nolint suppresses false positive in a library function
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    if (code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto module = std::make_unique<wasm::Module>();
    {
      wasm::WasmBinaryBuilder parser(
          *module,
          reinterpret_cast<const std::vector<char> &>(code),  // NOLINT
          false);

      try {
        parser.read();
      } catch (wasm::ParseException &e) {
        std::ostringstream msg;
        e.dump(msg);
        log->error(msg.str());
        return Error::INVALID_STATE_CODE;
      }
    }

    module->memory.initial = kDefaultHeappages;

    return std::make_shared<ModuleImpl>(
        std::move(module), module_factory, env_factory, code_hash);
  }

  outcome::result<std::shared_ptr<ModuleInstance>> ModuleImpl::instantiate()
      const {
    auto env = env_factory_->make(module_factory_);
    return std::make_shared<ModuleInstanceImpl>(
        std::move(env.env), shared_from_this(), env.rei, code_hash_);
  }

}  // namespace kagome::runtime::binaryen
