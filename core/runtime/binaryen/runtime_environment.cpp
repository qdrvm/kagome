/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_environment.hpp"

#include "crypto/hasher.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"

namespace kagome::runtime::binaryen {

  outcome::result<RuntimeEnvironment> RuntimeEnvironment::create(
      std::shared_ptr<RuntimeExternalInterface> rei,
      std::shared_ptr<WasmModule> module,

      const common::Buffer &state_code) {

    return RuntimeEnvironment{
        module->instantiate(rei), rei->memory(), boost::none};
  }

}  // namespace kagome::runtime::binaryen
