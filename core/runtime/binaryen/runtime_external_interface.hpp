/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP

#include <binaryen/shell-interface.h>

#include "common/logger.hpp"
#include "extensions/extension_factory.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface : public wasm::ShellExternalInterface {
   public:
    explicit RuntimeExternalInterface(
        std::shared_ptr<extensions::ExtensionFactory> extension_factory);

    wasm::Literal callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    inline std::shared_ptr<WasmMemory> memory() const {
      return extension_->memory();
    }

   private:
    /**
     * Checks that the number of arguments is as expected and terminates the
     * program if it is not
     */
    void checkArguments(std::string_view extern_name,
                        size_t expected,
                        size_t actual);

    std::shared_ptr<extensions::ExtensionFactory> extension_factory_;
    std::shared_ptr<extensions::Extension> extension_;
    common::Logger logger_ = common::createLogger(kDefaultLoggerTag);

    constexpr static auto kDefaultLoggerTag = "Runtime external interface";
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_EXTERNAL_INTERFACE_HPP
