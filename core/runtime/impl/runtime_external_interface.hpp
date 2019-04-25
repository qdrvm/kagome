/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_RUNTIME_EXTERNAL_INTERFACE_HPP
#define KAGOME_CORE_RUNTIME_IMPL_RUNTIME_EXTERNAL_INTERFACE_HPP

#include <binaryen/shell-interface.h>
#include "common/logger.hpp"
#include "extensions/extension.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  class RuntimeExternalInterface : public wasm::ShellExternalInterface {
   public:
    explicit RuntimeExternalInterface(
        std::shared_ptr<extensions::Extension> extension,
        common::Logger logger = common::createLogger(kDefaultLoggerTag));

    /**
     * Initializes the the external interface with memory from module and
     * globals from instance
     */
    void init(wasm::Module &wasm, wasm::ModuleInstance &instance) override;

    wasm::Literal callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    /**
     * Load integers from provided address
     */
    int8_t load8s(wasm::Address addr) override;
    uint8_t load8u(wasm::Address addr) override;
    int16_t load16s(wasm::Address addr) override;
    uint16_t load16u(wasm::Address addr) override;
    int32_t load32s(wasm::Address addr) override;
    uint32_t load32u(wasm::Address addr) override;
    int64_t load64s(wasm::Address addr) override;
    uint64_t load64u(wasm::Address addr) override;
    std::array<uint8_t, 16> load128(wasm::Address addr) override;

    /**
     * Store integers at given address of the wasm memory
     */
    void store8(wasm::Address addr, int8_t value) override;
    void store16(wasm::Address addr, int16_t value) override;
    void store32(wasm::Address addr, int32_t value) override;
    void store64(wasm::Address addr, int64_t value) override;
    void store128(wasm::Address addr,
                  const std::array<uint8_t, 16> &value) override;

    /**
     * Resize memory to the new size
     * @note first argument should be ignored, it is only to conform interface
     */
    void growMemory(wasm::Address /*oldSize*/, wasm::Address newSize) override;

   private:
    /**
     * Checks that the number of arguments is as expected and terminates the
     * program if it is not
     */
    void checkArguments(std::string_view extern_name, size_t expected,
                        size_t actual);

    std::shared_ptr<extensions::Extension> extension_;
    std::shared_ptr<WasmMemory> memory_;
    common::Logger logger_;

    constexpr static auto kDefaultLoggerTag = "Runtime external interface";
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_RUNTIME_EXTERNAL_INTERFACE_HPP
