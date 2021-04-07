/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_environment.hpp"

#include "crypto/hasher.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"
#include "runtime/binaryen/wasm_executor.hpp"
#include "runtime/wasm_memory.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime::binaryen {

  outcome::result<RuntimeEnvironment> RuntimeEnvironment::create(
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

    auto memory = rei->memory();
    memory->setHeapBase(heap_base);
    memory->reset();

    return RuntimeEnvironment{std::move(module_instance), std::move(memory)};
  }

}  // namespace kagome::runtime::binaryen
