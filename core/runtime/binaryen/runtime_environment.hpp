/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_ENVIRONMENT
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_ENVIRONMENT

#include <memory>

#include <boost/optional.hpp>

#include "outcome/outcome.hpp"

namespace kagome::common {
  class Buffer;
}

namespace kagome::storage::trie {
  class TopperTrieBatch;
  class TrieBatch;
}

namespace kagome::runtime {
  class WasmMemory;
}

namespace kagome::runtime::binaryen {
  class RuntimeExternalInterface;
  class WasmModuleInstance;
  class WasmModule;

  class RuntimeEnvironment {
   public:
    static outcome::result<RuntimeEnvironment> create(
        std::shared_ptr<RuntimeExternalInterface> rei,
        std::shared_ptr<WasmModule> module,
        const common::Buffer &state_code);

    RuntimeEnvironment(RuntimeEnvironment &&) = default;
    RuntimeEnvironment &operator=(RuntimeEnvironment &&) = default;

    RuntimeEnvironment(const RuntimeEnvironment &) = delete;
    RuntimeEnvironment &operator=(const RuntimeEnvironment &) = delete;

    std::shared_ptr<WasmModuleInstance> module_instance;
    std::shared_ptr<WasmMemory> memory;
    boost::optional<std::shared_ptr<storage::trie::TopperTrieBatch>>
        batch;  // in persistent environments all changes of a call must be
                // either applied together or discarded in case of failure
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_ENVIRONMENT
