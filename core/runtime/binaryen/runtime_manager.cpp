/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_manager.hpp"

#include <binaryen/wasm-binary.h>
#include <gsl/gsl>

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

  RuntimeManager::RuntimeManager(
      std::shared_ptr<runtime::WasmProvider> wasm_provider,
      std::shared_ptr<extensions::ExtensionFactory> persistent_extension_factory,
      std::shared_ptr<extensions::ExtensionFactory> ephemeral_extension_factory,
      std::shared_ptr<crypto::Hasher> hasher)
      : wasm_provider_(std::move(wasm_provider)),
        persistent_extension_factory_(std::move(persistent_extension_factory)),
        ephemeral_extension_factory_(std::move(ephemeral_extension_factory)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(wasm_provider_);
    BOOST_ASSERT(persistent_extension_factory_);
    BOOST_ASSERT(ephemeral_extension_factory_);
    BOOST_ASSERT(hasher_);
  }

  outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
                             std::shared_ptr<WasmMemory>>>
  RuntimeManager::getPersistentRuntimeEnvironment() {

    return getRuntimeEnvironment(persistent_extension_factory_);
  }

  outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
                             std::shared_ptr<WasmMemory>>>
  RuntimeManager::getEphemeralRuntimeEnvironment() {
    return getRuntimeEnvironment(ephemeral_extension_factory_);
  }

  outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
                             std::shared_ptr<WasmMemory>>>
  RuntimeManager::getRuntimeEnvironment(
      std::shared_ptr<extensions::ExtensionFactory> extension_factory) {
    const auto &state_code = wasm_provider_->getStateCode();

    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto hash = hasher_->twox_256(state_code);

    if (hash != state_code_hash_) {
      OUTCOME_TRY(module, prepareModule(state_code));
      module_ = std::move(module);
      external_interface_.reset();
      state_code_hash_ = hash;
    }

    if (!external_interface_) {
      external_interface_ =
          std::make_shared<RuntimeExternalInterface>(extension_factory);
    }

    return {std::make_shared<wasm::ModuleInstance>(*module_,
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
