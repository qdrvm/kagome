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
  class Memory;
  class ExecutorFactory;

}  // namespace kagome::runtime

namespace wasm {
  class Function;
}

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface : public wasm::ShellExternalInterface {
   public:
    explicit RuntimeExternalInterface(std::shared_ptr<host_api::HostApi> host_api);

    wasm::Literals callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    wasm::ShellExternalInterface::Memory* getMemory();

   private:
    /**
     * Checks that the number of arguments is as expected and terminates the
     * program if it is not
     */
    void checkArguments(std::string_view extern_name,
                        size_t expected,
                        size_t actual);

    std::shared_ptr<host_api::HostApi> host_api_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
