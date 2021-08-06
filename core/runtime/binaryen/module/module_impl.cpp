/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/module_impl.hpp"

#include <memory>

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>

#include "common/buffer.hpp"
#include "common/literals.hpp"
#include "common/mp_utils.hpp"
#include "runtime/binaryen/module/module_instance_impl.hpp"
#include "runtime/trie_storage_provider.hpp"
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
      std::shared_ptr<RuntimeExternalInterface> external_interface)
      : external_interface_{std::move(external_interface)},
        module_{std::move(module)} {
    BOOST_ASSERT(module_ != nullptr);
    BOOST_ASSERT(external_interface_ != nullptr);
  }

  outcome::result<std::unique_ptr<ModuleImpl>> ModuleImpl::createFromCode(
      const std::vector<uint8_t> &code,
      const std::shared_ptr<TrieStorageProvider> &storage_provider,
      std::shared_ptr<RuntimeExternalInterface> external_interface) {
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
          reinterpret_cast<std::vector<char> const &>(  // NOLINT
              code));

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
    OUTCOME_TRY(heappages_key, common::Buffer::fromString(":heappages"));
    auto heappages_res =
        storage_provider->getCurrentBatch()->get(heappages_key);
    if (heappages_res.has_value()) {
      auto &&heappages = heappages_res.value();
      if (sizeof(uint64_t) != heappages.size()) {
        log->error(
            "Unable to read :heappages value. Type size mismatch. "
            "Required {} bytes, but {} available",
            sizeof(uint64_t),
            heappages.size());
      } else {
        uint64_t pages = common::bytes_to_uint64_t(heappages.asVector());
        module->memory.initial = pages;
        log->trace(
            "Creating wasm module with non-default :heappages value set to {}",
            pages);
      }
    } else if (kagome::storage::trie::TrieError::NO_VALUE
               != heappages_res.error()) {
      return heappages_res.error();
    }
    std::unique_ptr<ModuleImpl> wasm_module_impl(
        new ModuleImpl(std::move(module), std::move(external_interface)));
    return wasm_module_impl;
  }

  outcome::result<std::unique_ptr<ModuleInstance>> ModuleImpl::instantiate()
      const {
    return std::make_unique<ModuleInstanceImpl>(module_, external_interface_);
  }

}  // namespace kagome::runtime::binaryen
