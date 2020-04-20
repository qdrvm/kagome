/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime_manager.hpp"

#include <binaryen/wasm-binary.h>

#include <crypto/hasher/hasher_impl.hpp>

#include "runtime_external_interface.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            RuntimeManager::Error,
                            e) {
  using kagome::runtime::binaryen::RuntimeManager;
  switch (e) {
    case RuntimeManager::Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case RuntimeManager::Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
  }
}

namespace kagome::runtime::binaryen {

  outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
                             std::shared_ptr<WasmMemory>>>
  RuntimeManager::getModuleInstance() {
    const auto &state_code = wasm_provider_->getStateCode();

    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto hash = kagome::crypto::HasherImpl().sha2_256(state_code);

    if (hash != state_code_hash_) {
      OUTCOME_TRY(module, prepareModule(state_code));
      module_ = std::move(module);
      external_interface_.reset();
      state_code_hash_ = hash;
    }

    if (!external_interface_) {
      external_interface_ =
          std::make_shared<RuntimeExternalInterface>(extension_factory_);
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
