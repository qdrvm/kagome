/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_manager.hpp"

#include <binaryen/wasm-binary.h>

#include "crypto/hasher/hasher_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            RuntimeManager::Error,
                            e) {
  using Error = kagome::runtime::binaryen::RuntimeManager::Error;
  switch (e) {
    case Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
  }
}

namespace kagome::runtime::binaryen {

  thread_local std::shared_ptr<RuntimeExternalInterface>
      RuntimeManager::external_interface_{};

  RuntimeManager::RuntimeManager(
      std::shared_ptr<runtime::WasmProvider> wasm_provider,
      std::shared_ptr<extensions::ExtensionFactory> extension_factory,
      std::shared_ptr<crypto::Hasher> hasher)
      : wasm_provider_(std::move(wasm_provider)),
        extension_factory_(std::move(extension_factory)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(wasm_provider_);
    BOOST_ASSERT(extension_factory_);
    BOOST_ASSERT(hasher_);
  }

  outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
                             std::shared_ptr<WasmMemory>>>
  RuntimeManager::getRuntimeEnvironment() {
    const auto &state_code = wasm_provider_->getStateCode();

    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto hash = hasher_->twox_256(state_code);

    std::shared_ptr<wasm::Module> module;

    // Trying retrieve pre-prepared module
    {
      std::lock_guard lockGuard(modules_mutex_);
      auto it = modules_.find(hash);
      if (it != modules_.end()) {
        module = it->second;
      }
    }

    if (!module) {
      // Prepare new module
      OUTCOME_TRY(new_module, prepareModule(state_code));

      // Trying to safe emplace new module, and use existed one
      //  if it already emplaced in another thread
      std::lock_guard lockGuard(modules_mutex_);
      module = modules_.emplace(std::move(hash), std::move(new_module))
                   .first->second;
    }

    if (!external_interface_) {
      external_interface_ =
          std::make_shared<RuntimeExternalInterface>(extension_factory_);
    }

    return {std::make_shared<wasm::ModuleInstance>(*module,
                                                   external_interface_.get()),
            external_interface_->memory()};
  }

  outcome::result<std::shared_ptr<wasm::Module>> RuntimeManager::prepareModule(
      const common::Buffer &state_code) {
    // that nolint supresses false positive in a library function
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto module = std::make_shared<wasm::Module>();
    wasm::WasmBinaryBuilder parser(
        *module,
        reinterpret_cast<std::vector<char> const &>(  // NOLINT
            state_code.toVector()),
        false);

    try {
      parser.read();
    } catch (wasm::ParseException &e) {
      std::ostringstream msg;
      e.dump(msg);
      logger_->error(msg.str());
      return Error::INVALID_STATE_CODE;
    }
    return module;
  }

}  // namespace kagome::runtime::binaryen
