/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_environment.hpp"

#include "crypto/hasher.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"
#include "runtime/binaryen/wasm_executor.hpp"

namespace kagome::runtime::binaryen {

  outcome::result<RuntimeEnvironment> RuntimeEnvironment::create(
      const std::shared_ptr<RuntimeExternalInterface> &rei,
      const std::shared_ptr<WasmModule> &module) {
    auto module_instance = module->instantiate(rei);

    WasmExecutor executor;
    OUTCOME_TRY(heap_base, executor.get(*module_instance, "__heap_base"));

    auto memory = rei->memory();
    memory->setHeapBase(heap_base.geti32());

    return RuntimeEnvironment{std::move(module_instance), std::move(memory)};
  }

}  // namespace kagome::runtime::binaryen
