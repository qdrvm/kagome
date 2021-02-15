/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP

#include <binaryen/shell-interface.h>

#include "common/logger.hpp"
#include "host_api/host_api_factory.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"

namespace kagome::runtime::binaryen {

  class WasmMemoryImpl;  // not fancy to refer to impl, but have to do it  for
                         // now because it depends on shell interface memory
                         // belonging to RuntimeExternalInterface (see
                         // constructor)

  class RuntimeExternalInterface : public wasm::ShellExternalInterface {
   public:
    explicit RuntimeExternalInterface(
        std::shared_ptr<BinaryenWasmMemoryFactory> wasm_memory_factory,
        const std::shared_ptr<host_api::HostApiFactory> &host_api_factory,
        std::shared_ptr<TrieStorageProvider> storage_provider);

    wasm::Literal callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    inline std::shared_ptr<WasmMemory> memory() const {
      return host_api_->memory();
    }

    inline void reset() const {
      return host_api_->reset();
    }

   private:
    /**
     * Checks that the number of arguments is as expected and terminates the
     * program if it is not
     */
    void checkArguments(std::string_view extern_name,
                        size_t expected,
                        size_t actual);

    std::unique_ptr<host_api::HostApi> host_api_;
    common::Logger logger_ = common::createLogger(kDefaultLoggerTag);

    constexpr static auto kDefaultLoggerTag = "Runtime external interface";
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
