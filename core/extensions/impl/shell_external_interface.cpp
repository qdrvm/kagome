/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/shell_external_interface.hpp"

namespace kagome::extensions {

  using namespace wasm;

  void ShellExternalInterface::init(wasm::Module &wasm,
                                    wasm::ModuleInstance &instance) {
    memory.resize(wasm.memory.initial * wasm::Memory::kPageSize);
    // apply memory segments
    for (auto &segment : wasm.memory.segments) {
      wasm::Address offset =
          (uint32_t)ConstantExpressionRunner<TrivialGlobalManager>(
              instance.globals)
              .visit(segment.offset)
              .value.geti32();
      if (offset + segment.data.size()
          > wasm.memory.initial * wasm::Memory::kPageSize) {
        trap("invalid offset when initializing memory");
      }
      for (size_t i = 0; i != segment.data.size(); ++i) {
        memory.set(offset + i, segment.data[i]);
      }
    }

    table.resize(wasm.table.initial);
    for (auto &segment : wasm.table.segments) {
      wasm::Address offset =
          (uint32_t)ConstantExpressionRunner<TrivialGlobalManager>(
              instance.globals)
              .visit(segment.offset)
              .value.geti32();
      if (offset + segment.data.size() > wasm.table.initial) {
        trap("invalid offset when initializing table");
      }
      for (size_t i = 0; i != segment.data.size(); ++i) {
        table[offset + i] = segment.data[i];
      }
    }
  }

  void ShellExternalInterface::importGlobals(
      std::map<wasm::Name, wasm::Literal> &globals, wasm::Module &wasm) {
    // add spectest globals
    ModuleUtils::iterImportedGlobals(wasm, [&](wasm::Global *import) {
      // add your code to work with globals here
    });
  }

  Literal ShellExternalInterface::callImport(Function *import,
                                             LiteralList &arguments) {
    if (import->module == "env" && import->base == "ext_malloc") {
      auto size_arg = *arguments.begin();
      auto opt_address = memory.allocate(size_arg.geti64());
      if (opt_address) {
        return Literal{(uint64_t)opt_address.value()};
      }
      return Literal(int64_t{-1});
    }
    Fatal() << "callImport: unknown import: " << import->module.str << "."
            << import->name.str;
  }

  Literal ShellExternalInterface::callTable(Index index, LiteralList &arguments,
                                            Type result,
                                            ModuleInstance &instance) {
    if (index >= table.size())
      trap("callTable overflow");
    auto *func = instance.wasm.getFunctionOrNull(table[index]);
    if (!func)
      trap("uninitialized table element");
    if (func->params.size() != arguments.size())
      trap("callIndirect: bad # of arguments");
    for (size_t i = 0; i < func->params.size(); i++) {
      if (func->params[i] != arguments[i].type) {
        trap("callIndirect: bad argument type");
      }
    }
    if (func->result != result) {
      trap("callIndirect: bad result type");
    }
    if (func->imported()) {
      return callImport(func, arguments);
    } else {
      return instance.callFunctionInternal(func->name, arguments);
    }
  }

  int8_t ShellExternalInterface::load8s(Address addr) {
    return memory.get<int8_t>(addr);
  }

  uint8_t ShellExternalInterface::load8u(Address addr) {
    return memory.get<uint8_t>(addr);
  }

  int16_t ShellExternalInterface::load16s(Address addr) {
    return memory.get<int16_t>(addr);
  }

  uint16_t ShellExternalInterface::load16u(Address addr) {
    return memory.get<uint16_t>(addr);
  }

  int32_t ShellExternalInterface::load32s(wasm::Address addr) {
    return memory.get<int32_t>(addr);
  }

  uint32_t ShellExternalInterface::load32u(Address addr) {
    return memory.get<uint32_t>(addr);
  }

  int64_t ShellExternalInterface::load64s(Address addr) {
    return memory.get<int64_t>(addr);
  }

  uint64_t ShellExternalInterface::load64u(Address addr) {
    return memory.get<uint64_t>(addr);
  }

  std::array<uint8_t, 16> ShellExternalInterface::load128(Address addr) {
    return memory.get<std::array<uint8_t, 16>>(addr);
  }

  void ShellExternalInterface::store8(Address addr, int8_t value) {
    memory.set<int8_t>(addr, value);
  }

  void ShellExternalInterface::store16(Address addr, int16_t value) {
    memory.set<int16_t>(addr, value);
  }

  void ShellExternalInterface::store32(Address addr, int32_t value) {
    memory.set<int32_t>(addr, value);
  }

  void ShellExternalInterface::store64(Address addr, int64_t value) {
    memory.set<int64_t>(addr, value);
  }

  void ShellExternalInterface::store128(Address addr,
                                        const std::array<uint8_t, 16> &value) {
    memory.set<std::array<uint8_t, 16>>(addr, value);
  }

  void ShellExternalInterface::growMemory(Address, Address newSize) {
    memory.resize(newSize);
  }

  void ShellExternalInterface::trap(const char *why) {
    std::cout << "[trap " << why << "]\n";
    throw std::exception();
  }

}  // namespace kagome::extensions
