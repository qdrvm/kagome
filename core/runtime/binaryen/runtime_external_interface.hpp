/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <binaryen/shell-interface.h>

#include <boost/unordered_map.hpp>

#include "common/buffer_view.hpp"
#include "log/logger.hpp"
#include "runtime/memory_check.hpp"

namespace kagome::host_api {
  class HostApiFactory;
  class HostApi;
}  // namespace kagome::host_api

namespace kagome::runtime {
  using BytesOut = std::span<uint8_t>;

  class TrieStorageProvider;
  class Memory;
  class ExecutorFactory;

}  // namespace kagome::runtime

namespace wasm {
  class Function;
}

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface
      : public wasm::ModuleInstance::ExternalInterface {
    class Memory {
      friend class RuntimeExternalInterface;

      using Mem = std::vector<char>;
      Mem memory;
      std::optional<WasmSize> pages_max;
      template <typename T>
      static bool aligned(const char *address) {
        static_assert(!(sizeof(T) & (sizeof(T) - 1)), "must be a power of 2");
        return 0 == (reinterpret_cast<uintptr_t>(address) & (sizeof(T) - 1));
      }
      Memory(Memory &) = delete;
      Memory &operator=(const Memory &) = delete;

     public:
      Memory() = default;
      uint32_t pages() {
        return memory.size() / ::wasm::Memory::kPageSize;
      }
      void pagesResize(size_t newPages) {
        size_t newSize = newPages * ::wasm::Memory::kPageSize;
        // Ensure the smallest allocation is large enough that most allocators
        // will provide page-aligned storage. This hopefully allows the
        // interpreter's memory to be as aligned as the memory being simulated,
        // ensuring that the performance doesn't needlessly degrade.
        //
        // The code is optimistic this will work until WG21's p0035r0 happens.
        const size_t minSize = 1 << 12;
        size_t oldSize = memory.size();
        memory.resize(std::max(minSize, newSize));
        if (newSize < oldSize && newSize < minSize) {
          std::memset(&memory[newSize], 0, minSize - newSize);
        }
      }
      auto getSize() const {
        return memory.size();
      }
      std::optional<WasmSize> pagesMax() const {
        return pages_max;
      }
      template <typename T>
      void set(size_t address, T value) {
        check(address, sizeof(T));
        if (aligned<T>(&memory[address])) {
          *reinterpret_cast<T *>(&memory[address]) = value;
        } else {
          std::memcpy(&memory[address], &value, sizeof(T));
        }
      }
      template <typename T>
      T get(size_t address) {
        check(address, sizeof(T));
        if (aligned<T>(&memory[address])) {
          return *reinterpret_cast<T *>(&memory[address]);
        } else {
          T loaded;
          std::memcpy(&loaded, &memory[address], sizeof(T));
          return loaded;
        }
      }

      void check(WasmPointer ptr, WasmSize size) {
        if (not memoryCheck(ptr, size, memory.size())) {
          throw std::out_of_range{"MemoryError"};
        }
      }

      BytesOut view(WasmPointer ptr, WasmSize size) {
        check(ptr, size);
        return {reinterpret_cast<uint8_t *>(&memory[ptr]), size};
      }
    } memory;

    std::vector<wasm::Name> table;

   public:
    using InternalMemory = Memory;

    explicit RuntimeExternalInterface(
        std::shared_ptr<host_api::HostApi> host_api);

    void init(wasm::Module &wasm, wasm::ModuleInstance &instance) override;
    void importGlobals(std::map<wasm::Name, wasm::Literal> &globals,
                       wasm::Module &wasm) override;
    wasm::Literal callTable(wasm::Index index,
                            wasm::LiteralList &arguments,
                            wasm::Type result,
                            wasm::ModuleInstance &instance) override;

    wasm::Literal callImport(wasm::Function *import,
                             wasm::LiteralList &arguments) override;

    InternalMemory *getMemory();

    int8_t load8s(wasm::Address addr) override {
      return memory.get<int8_t>(addr);
    }
    uint8_t load8u(wasm::Address addr) override {
      return memory.get<uint8_t>(addr);
    }
    int16_t load16s(wasm::Address addr) override {
      return memory.get<int16_t>(addr);
    }
    uint16_t load16u(wasm::Address addr) override {
      return memory.get<uint16_t>(addr);
    }
    int32_t load32s(wasm::Address addr) override {
      return memory.get<int32_t>(addr);
    }
    uint32_t load32u(wasm::Address addr) override {
      return memory.get<uint32_t>(addr);
    }
    int64_t load64s(wasm::Address addr) override {
      return memory.get<int64_t>(addr);
    }
    uint64_t load64u(wasm::Address addr) override {
      return memory.get<uint64_t>(addr);
    }
    std::array<uint8_t, 16> load128(wasm::Address addr) override {
      return memory.get<std::array<uint8_t, 16>>(addr);
    }

    void store8(wasm::Address addr, int8_t value) override {
      memory.set<int8_t>(addr, value);
    }
    void store16(wasm::Address addr, int16_t value) override {
      memory.set<int16_t>(addr, value);
    }
    void store32(wasm::Address addr, int32_t value) override {
      memory.set<int32_t>(addr, value);
    }
    void store64(wasm::Address addr, int64_t value) override {
      memory.set<int64_t>(addr, value);
    }
    void store128(wasm::Address addr,
                  const std::array<uint8_t, 16> &value) override {
      memory.set<std::array<uint8_t, 16>>(addr, value);
    }

    uint32_t memoryPages() override {
      return memory.pages();
    }

    void memoryPagesGrow(uint32_t /*oldPages*/, uint32_t newPages) override {
      memory.pagesResize(newPages);
    }

    [[noreturn]] void trap(const char *why) override {
      logger_->error("Trap: {}", why);
      throw wasm::TrapException{};
    }

   private:
    /**
     * Checks that the number of arguments is as expected and terminates the
     * program if it is not
     */
    void checkArguments(std::string_view extern_name,
                        size_t expected,
                        size_t actual);

    void registerMethods();

    template <auto mf>
    static wasm::Literal importCall(RuntimeExternalInterface &this_,
                                    wasm::Function *import,
                                    wasm::LiteralList &arguments);

    std::shared_ptr<host_api::HostApi> host_api_;

    using ImportFuncPtr = wasm::Literal (*)(RuntimeExternalInterface &this_,
                                            wasm::Function *import,
                                            wasm::LiteralList &arguments);

    boost::unordered_map<std::string, ImportFuncPtr> imports_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::binaryen
