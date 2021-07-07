/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_environment.hpp"

#include "crypto/hasher.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"
#include "runtime/binaryen/wasm_executor.hpp"
#include "runtime/memory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime::binaryen {

  RuntimeEnvironment::RuntimeEnvironment(
      Memory &memory,
      const std::shared_ptr<RuntimeExternalInterface> &rei,
      const std::shared_ptr<WasmModuleInstance> &module_instance,
      boost::optional<std::shared_ptr<storage::trie::TopperTrieBatch>> batch)
      : module_instance{module_instance},
        memory{memory},
        rei{rei},
        batch{batch} {}

  RuntimeEnvironment::RuntimeEnvironment(RuntimeEnvironment &&re)
      : module_instance{std::move(re.module_instance)},
        memory{re.memory},
        rei{std::move(re.rei)},
        batch{std::move(re.batch)} {}

  RuntimeEnvironment &RuntimeEnvironment::operator=(RuntimeEnvironment &&re) {
    module_instance = std::move(re.module_instance);
    memory = re.memory;
    rei = std::move(re.rei);
    batch = std::move(re.batch);
    return *this;
  }

  outcome::result<RuntimeEnvironment> RuntimeEnvironment::create(
      const std::shared_ptr<MemoryProvider> &memory_provider,
      const std::shared_ptr<RuntimeExternalInterface> &rei,
      const std::shared_ptr<WasmModule> &module) {
    auto module_instance = module->instantiate(rei);

    WasmExecutor executor;
    WasmPointer heap_base;

    auto heap_base_res = executor.get(*module_instance, "__heap_base");
    if (heap_base_res.has_value()) {
      heap_base = heap_base_res.value().geti32();
    } else {
      heap_base = 1024;  // Fallback value
    }

    memory_provider->resetMemory(heap_base);
    auto memory = memory_provider->getCurrentMemory();
    BOOST_ASSERT(memory.has_value());

    return outcome::success(RuntimeEnvironment{
        memory.value(), rei, std::move(module_instance), boost::none});
  }

  RuntimeEnvironment::~RuntimeEnvironment() {
    if (rei) {
      rei->reset();
    }
  }

}  // namespace kagome::runtime::binaryen
