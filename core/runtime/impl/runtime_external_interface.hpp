/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_RUNTIME_EXTERNAL_INTERFACE_HPP_
#define KAGOME_CORE_RUNTIME_IMPL_RUNTIME_EXTERNAL_INTERFACE_HPP_

#include <binaryen/shell-interface.h>

#include "extensions/extension.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  class RuntimeExternalInterface : public wasm::ShellExternalInterface {
   public:
    explicit RuntimeExternalInterface(
        std::shared_ptr<extensions::Extension> extension,
        std::shared_ptr<WasmMemory> memory);

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

    void growMemory(wasm::Address /*oldSize*/, wasm::Address newSize) override;

   private:
    std::shared_ptr<extensions::Extension> extension_;
    std::shared_ptr<WasmMemory> memory_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_RUNTIME_EXTERNAL_INTERFACE_HPP_
