/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP

#include <binaryen/shell-interface.h>

#include "log/logger.hpp"

namespace kagome::host_api {
  class HostApiFactory;
  class HostApi;
}

namespace kagome::runtime {
  class TrieStorageProvider;
  class WasmMemory;

  namespace binaryen {
    class CoreFactory;
    class RuntimeEnvironmentFactory;
    class BinaryenWasmMemoryFactory;
  }

}  // namespace kagome::runtime

namespace wasm {
  class Function;
}

namespace kagome::runtime::binaryen {

  class WasmMemoryImpl;  // not fancy to refer to impl, but have to do it  for
                         // now because it depends on shell interface memory
                         // belonging to RuntimeExternalInterface (see
                         // constructor)

  class RuntimeExternalInterface : public wasm::ShellExternalInterface {
   public:
    RuntimeExternalInterface(
        std::shared_ptr<CoreFactory> core_factory,
        std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory,
        std::shared_ptr<BinaryenWasmMemoryFactory> wasm_memory_factory,
        const std::shared_ptr<host_api::HostApiFactory> &host_api_factory,
        std::shared_ptr<TrieStorageProvider> storage_provider);

    wasm::Literal callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    std::shared_ptr<WasmMemory> memory() const;

    void reset() const;

   private:
    /**
     * Checks that the number of arguments is as expected and terminates the
     * program if it is not
     */
    void checkArguments(std::string_view extern_name,
                        size_t expected,
                        size_t actual);

    std::unique_ptr<host_api::HostApi> host_api_;
    log::Logger logger_ = log::createLogger("RuntimeExternalInterface", "wasm");
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
