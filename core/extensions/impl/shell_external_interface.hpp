/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

//
// Implementation of the shell interpreter execution environment with
// allocatable memory
//

#ifndef wasm_shell_interface_h
#define wasm_shell_interface_h

#include <asmjs/shared-constants.h>
#include <binaryen/wasm.h>
#include <ir/module-utils.h>
#include <shared-constants.h>
#include <support/name.h>
#include <wasm-interpreter.h>
#include <wasm.h>

#include "extensions/extension.hpp"
#include "extensions/impl/memory.hpp"

namespace kagome::extensions {

  struct ShellExternalInterface : wasm::ModuleInstance::ExternalInterface {
   private:
    kagome::Memory memory;
    std::vector<wasm::Name> table;

    std::shared_ptr<Extension> extensions_;

   public:
    ShellExternalInterface() : memory() {}
    virtual ~ShellExternalInterface() = default;

    void init(wasm::Module &wasm, wasm::ModuleInstance &instance) override;

    void importGlobals(std::map<wasm::Name, wasm::Literal> &globals,
                       wasm::Module &wasm) override;

    wasm::Literal callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    wasm::Literal callTable(wasm::Index index, wasm::LiteralList &arguments,
                            wasm::Type result,
                            wasm::ModuleInstance &instance) override;

    int8_t load8s(wasm::Address addr) override;

    uint8_t load8u(wasm::Address addr) override;

    int16_t load16s(wasm::Address addr) override;

    uint16_t load16u(wasm::Address addr) override;

    int32_t load32s(wasm::Address addr) override;

    uint32_t load32u(wasm::Address addr) override;

    int64_t load64s(wasm::Address addr) override;

    uint64_t load64u(wasm::Address addr) override;
    std::array<uint8_t, 16> load128(wasm::Address addr) override;

    void store8(wasm::Address addr, int8_t value) override;
    void store16(wasm::Address addr, int16_t value) override;
    void store32(wasm::Address addr, int32_t value) override;
    void store64(wasm::Address addr, int64_t value) override;
    void store128(wasm::Address addr,
                  const std::array<uint8_t, 16> &value) override;

    void growMemory(wasm::Address /*oldSize*/, wasm::Address newSize) override;

    void trap(const char *why) override;
  };

}  // namespace kagome::extensions

#endif  // wasm_shell_interface_h
