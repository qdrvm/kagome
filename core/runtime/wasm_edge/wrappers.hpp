/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::runtime::wasm_edge {

  std::atomic_int module_instance_count = 0;
  std::atomic_int module_count = 0;
  std::atomic_int memory_instance_count = 0;
  std::atomic_int executor_count = 0;
  std::atomic_int misc_count = 0;

  template <typename T, auto deleter, std::atomic_int &count = misc_count>
    requires std::invocable<decltype(deleter), T>
  class Wrapper {
   public:
    Wrapper() : t{} {
      count++;
      std::cout << "Create " << typeid(T).name() << ", total " << count << "\n";
    }

    Wrapper(T t) : t{std::move(t)} {
      count++;
      std::cout << "Create " << typeid(T).name() << ", total " << count << "\n";
    }

    Wrapper(const Wrapper &) = delete;
    Wrapper &operator=(const Wrapper &) = delete;

    Wrapper(Wrapper &&other) {
      if (t) {
        deleter(t);
      }
      t = other.t;
      other.t = nullptr;
    }

    Wrapper &operator=(Wrapper &&other) {
      if (t) {
        deleter(t);
      }
      t = other.t;
      other.t = nullptr;
      return *this;
    }

    ~Wrapper() {
      if constexpr (std::is_pointer_v<T>) {
        if (t) {
          deleter(t);
          count--;
          std::cout << "Destroy " << typeid(T).name() << ", total " << count
                    << "\n";
        }
      } else {
        deleter(t);
        count--;
        std::cout << "Destroy " << typeid(T).name() << ", total " << count
                  << "\n";
      }
    }

    T &raw() {
      return t;
    }

    const T &raw() const {
      return t;
    }

    T *operator->() {
      return t;
    }

    const T *operator->() const {
      return t;
    }

    const T *operator&() const {
      return &t;
    }

    T *operator&() {
      return &t;
    }

    bool operator==(std::nullptr_t) const {
      return t == nullptr;
    }

    bool operator!=(std::nullptr_t) const {
      return t != nullptr;
    }

    T t = nullptr;
  };

  using ConfigureContext =
      Wrapper<WasmEdge_ConfigureContext *, WasmEdge_ConfigureDelete>;
  using LoaderContext =
      Wrapper<WasmEdge_LoaderContext *, WasmEdge_LoaderDelete>;
  using CompilerContext =
      Wrapper<WasmEdge_CompilerContext *, WasmEdge_CompilerDelete>;
  using StatsContext =
      Wrapper<WasmEdge_StatisticsContext *, WasmEdge_StatisticsDelete>;
  using FunctionTypeContext =
      Wrapper<WasmEdge_FunctionTypeContext *, WasmEdge_FunctionTypeDelete>;
  using FunctionInstanceContext = Wrapper<WasmEdge_FunctionInstanceContext *,
                                          WasmEdge_FunctionInstanceDelete>;
  using ExecutorContext = Wrapper<WasmEdge_ExecutorContext *,
                                  WasmEdge_ExecutorDelete,
                                  executor_count>;
  using StoreContext = Wrapper<WasmEdge_StoreContext *, WasmEdge_StoreDelete>;
  using ModuleInstanceContext = Wrapper<WasmEdge_ModuleInstanceContext *,
                                        WasmEdge_ModuleInstanceDelete,
                                        module_instance_count>;
  using String = Wrapper<WasmEdge_String, WasmEdge_StringDelete>;
  using ASTModuleContext = Wrapper<WasmEdge_ASTModuleContext *,
                                   WasmEdge_ASTModuleDelete,
                                   module_count>;
  using MemoryInstanceContext = Wrapper<WasmEdge_MemoryInstanceContext *,
                                        WasmEdge_MemoryInstanceDelete,
                                        memory_instance_count>;
  using ValidatorContext =
      Wrapper<WasmEdge_ValidatorContext *, WasmEdge_ValidatorDelete>;

}  // namespace kagome::runtime::wasm_edge
