/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/wasm_module_impl.hpp"

#include <memory>

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>

#include "common/buffer.hpp"
#include "common/literals.hpp"
#include "common/mp_utils.hpp"
#include "runtime/binaryen/module/wasm_module_instance_impl.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

using namespace kagome::common::literals;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            WasmModuleImpl::Error,
                            e) {
  using Error = kagome::runtime::binaryen::WasmModuleImpl::Error;
  switch (e) {
    case Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
  }
  return "Unknown error";
}

namespace kagome::runtime::binaryen {

  WasmModuleImpl::WasmModuleImpl(std::unique_ptr<wasm::Module> &&module)
      : module_{std::move(module)} {
    BOOST_ASSERT(module_ != nullptr);
  }

  outcome::result<std::unique_ptr<WasmModuleImpl>>
  WasmModuleImpl::createFromCode(
      const common::Buffer &code,
      const std::shared_ptr<RuntimeExternalInterface> &rei,
      const std::shared_ptr<TrieStorageProvider> &storage_provider) {
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
              code.asVector()),
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
    std::unique_ptr<WasmModuleImpl> wasm_module_impl(
        new WasmModuleImpl(std::move(module)));
    return wasm_module_impl;
  }

  std::unique_ptr<WasmModuleInstance> WasmModuleImpl::instantiate(
      const std::shared_ptr<RuntimeExternalInterface> &externalInterface)
      const {
    return std::make_unique<WasmModuleInstanceImpl>(module_, externalInterface);
  }

}  // namespace kagome::runtime::binaryen
