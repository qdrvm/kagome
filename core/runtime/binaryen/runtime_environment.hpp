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
}  // namespace kagome::storage::trie

namespace kagome::runtime {
  class Memory;
  class MemoryProvider;
}

namespace kagome::runtime::binaryen {;
  class WasmModuleInstance;
  class WasmModule;
  class RuntimeExternalInterface;

  /**
   * Runtime environment is a structure that contains data necessary to operate
   * with Runtime: memory, WASM module, storage batch
   */
  class RuntimeEnvironment {
   public:
    static outcome::result<RuntimeEnvironment> create(
        const std::shared_ptr<MemoryProvider> &memory_provider,
        const std::shared_ptr<RuntimeExternalInterface> &rei,
        const std::shared_ptr<WasmModule> &module);

    RuntimeEnvironment(RuntimeEnvironment &&);
    RuntimeEnvironment &operator=(RuntimeEnvironment &&);

    RuntimeEnvironment(const RuntimeEnvironment &) = delete;
    RuntimeEnvironment &operator=(const RuntimeEnvironment &) = delete;

    ~RuntimeEnvironment();

    std::shared_ptr<WasmModuleInstance> module_instance;
    Memory& memory;
    std::shared_ptr<RuntimeExternalInterface> rei;
    boost::optional<std::shared_ptr<storage::trie::TopperTrieBatch>>
        batch{};  // in persistent environments all changes of a call must be
                  // either applied together or discarded in case of failure
   private:
    RuntimeEnvironment(Memory& memory,
                       const std::shared_ptr<RuntimeExternalInterface> &rei,
                       const std::shared_ptr<WasmModuleInstance> &module_instance,
                       boost::optional<std::shared_ptr<storage::trie::TopperTrieBatch>> batch);
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_ENVIRONMENT
