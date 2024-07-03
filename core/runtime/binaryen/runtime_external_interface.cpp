/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_external_interface.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/common/register_host_api.hpp"
#include "runtime/memory.hpp"

namespace {
  /**
   * @brief one-to-one match of literal method to
   * C++ type
   */
  template <typename T>
  auto literalMemFun() {
    return nullptr;
  }

  template <>
  auto literalMemFun<int32_t>() {
    return &wasm::Literal::geti32;
  }

  template <>
  auto literalMemFun<uint32_t>() {
    return &wasm::Literal::geti32;
  }

  template <>
  auto literalMemFun<int64_t>() {
    return &wasm::Literal::geti64;
  }

  template <>
  auto literalMemFun<uint64_t>() {
    return &wasm::Literal::geti64;
  }

  /**
   * @brief a meta-layer that places list of arguments into host api method
   * invoсation using fold expression
   */
  template <typename T, typename R, auto mf, typename... Args, size_t... I>
  wasm::Literal callInternal(T *host_api,
                             const wasm::LiteralList &arguments,
                             std::index_sequence<I...>) {
    if constexpr (not std::is_same_v<void, R>) {
      // invokes literalMemFun method for every literal in a list to convert
      // wasm value into C++ value and pass it as argument
      return wasm::Literal(
          (host_api->*mf)((arguments.at(I).*literalMemFun<Args>())()...));
    } else {
      (host_api->*mf)((arguments.at(I).*literalMemFun<Args>())()...);
      return wasm::Literal();
    }
  }

  /**
   * @brief general HostApi method wrapper
   * Gets method type and value as template arguments. Has two separate
   * specializations for const and non-const HostApi methods
   */
  template <typename T, T>
  struct HostApiFunc;
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...)>
  struct HostApiFunc<R (T::*)(Args...), mf> {
    using Ret = R;
    static const size_t size = sizeof...(Args);
    static wasm::Literal call(T *host_api, const wasm::LiteralList &arguments) {
      // set index for every argument
      auto indices = std::index_sequence_for<Args...>{};
      return callInternal<T, R, mf, Args...>(host_api, arguments, indices);
    }
  };
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...) const>
  struct HostApiFunc<R (T::*)(Args...) const, mf> {
    using Ret = R;
    static const size_t size = sizeof...(Args);
    static wasm::Literal call(T *host_api, const wasm::LiteralList &arguments) {
      // set index for every argument
      auto indices = std::index_sequence_for<Args...>{};
      return callInternal<T, R, mf, Args...>(host_api, arguments, indices);
    }
  };

  /**
   * @brief gets number of arguments of method
   * @tparam mf method ref to invoke
   * @return argument list size
   */
  template <auto mf>
  constexpr size_t hostApiFuncArgSize() {
    return HostApiFunc<decltype(mf), mf>::size;
  }

  /**
   * @brief invokes host api method passed as a template argument
   * @tparam mf method ref to invoke
   * @param host_api HostApi implementation pointer
   * @param arguments list of arguments to invoke method with
   * @return implementation dependent anytype result
   */
  template <auto mf>
  wasm::Literal callHostApiFunc(kagome::host_api::HostApi *host_api,
                                const wasm::LiteralList &arguments) {
    return HostApiFunc<decltype(mf), mf>::call(host_api, arguments);
  }
}  // namespace

/**
 * @brief check arg num and call HostApi method macro
 * Aims to reduce boiler plate code and method name mentions. Uses name argument
 * as a string and a template argument.
 * @param name method name
 * @return result of method invocation
 */
#define REGISTER_HOST_METHOD(Ret, name, ...) \
  imports_[#name] = &importCall<&host_api::HostApi ::name>;

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::runtime::binaryen {

  static const wasm::Name env = "env";
  /**
   * @note: some implementation details were taken from
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h
   */

  void RuntimeExternalInterface::registerMethods() {
    REGISTER_HOST_METHODS;
  }

  RuntimeExternalInterface::RuntimeExternalInterface(
      std::shared_ptr<host_api::HostApi> host_api)
      : host_api_{std::move(host_api)},
        logger_{log::createLogger("RuntimeExternalInterface", "binaryen")} {
    BOOST_ASSERT(host_api_);
    registerMethods();
  }

  RuntimeExternalInterface::InternalMemory *
  RuntimeExternalInterface::getMemory() {
    return &memory;
  }

  wasm::Literal RuntimeExternalInterface::callImport(
      wasm::Function *import, wasm::LiteralList &arguments) {
    SL_TRACE(logger_, "Call import {}", import->base.str);
    if (import->module == env) {
      auto it = imports_.find(
          import->base.c_str(), imports_.hash_function(), imports_.key_eq());
      if (it != imports_.end()) {
        return it->second(*this, import, arguments);
      }
    }

    trap(fmt::format("Unknown Host method called: {}.{}",
                     import->module.str,
                     import->name.str)
             .c_str());
    return {};
  }

  void RuntimeExternalInterface::checkArguments(std::string_view extern_name,
                                                size_t expected,
                                                size_t actual) {
    if (expected != actual) {
      logger_->error(
          "Wrong number of arguments in {}. Expected: {}. Actual: {}",
          extern_name,
          expected,
          actual);
      throw std::runtime_error(
          "Invocation of a Host API method with wrong number of arguments");
    }
  }

  template <auto mf>
  wasm::Literal RuntimeExternalInterface::importCall(
      RuntimeExternalInterface &this_,
      wasm::Function *import,
      wasm::LiteralList &arguments) {
    this_.checkArguments(
        import->base.c_str(), hostApiFuncArgSize<mf>(), arguments.size());
    return callHostApiFunc<mf>(this_.host_api_.get(), arguments);
  }

  void RuntimeExternalInterface::init(wasm::Module &wasm,
                                      wasm::ModuleInstance &instance) {
    memory.pagesResize(wasm.memory.initial);
    if (wasm.memory.hasMax()) {
      memory.pages_max = wasm.memory.max;
    }
    // apply memory segments
    for (auto &segment : wasm.memory.segments) {
      wasm::Address offset =
          (uint32_t)wasm::ConstantExpressionRunner<wasm::TrivialGlobalManager>(
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
          (uint32_t)wasm::ConstantExpressionRunner<wasm::TrivialGlobalManager>(
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

  void RuntimeExternalInterface::importGlobals(
      std::map<wasm::Name, wasm::Literal> &globals, wasm::Module &wasm) {
    // add spectest globals
    wasm::ModuleUtils::iterImportedGlobals(wasm, [&](wasm::Global *import) {
      if (import->module == wasm::SPECTEST && import->base == wasm::GLOBAL) {
        switch (import->type) {
          case wasm::i32:
            globals[import->name] = wasm::Literal(int32_t(666));
            break;
          case wasm::i64:
            globals[import->name] = wasm::Literal(int64_t(666));
            break;
          case wasm::f32:
            globals[import->name] = wasm::Literal(float(666.6));
            break;
          case wasm::f64:
            globals[import->name] = wasm::Literal(double(666.6));
            break;
          case wasm::v128:
            assert(false && "v128 not implemented yet");
          case wasm::none:
          case wasm::unreachable:
            WASM_UNREACHABLE();
        }
      }
    });
    if (wasm.memory.imported() && wasm.memory.module == wasm::SPECTEST
        && wasm.memory.base == wasm::MEMORY) {
      // imported memory has initial 1 and max 2
      wasm.memory.initial = 1;
      wasm.memory.max = 2;
    }
  }

  wasm::Literal RuntimeExternalInterface::callTable(
      wasm::Index index,
      wasm::LiteralList &arguments,
      wasm::Type result,
      wasm::ModuleInstance &instance) {
    if (index >= table.size()) {
      trap("callTable overflow");
    }
    auto *func = instance.wasm.getFunctionOrNull(table[index]);
    if (!func) {
      trap("uninitialized table element");
    }
    if (func->params.size() != arguments.size()) {
      trap("callIndirect: bad # of arguments");
    }
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

}  // namespace kagome::runtime::binaryen
