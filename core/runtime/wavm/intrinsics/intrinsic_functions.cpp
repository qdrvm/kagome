/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"

#include <unordered_set>

#include "runtime/common/register_host_api.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"

namespace kagome::runtime::wavm {

  log::Logger logger;

  static thread_local std::stack<std::shared_ptr<ModuleInstance>>
      global_instances;

  void pushBorrowedRuntimeInstance(
      std::shared_ptr<ModuleInstance> borrowed_runtime_instance) {
    global_instances.emplace(std::move(borrowed_runtime_instance));
  }

  void popBorrowedRuntimeInstance() {
    BOOST_ASSERT(!global_instances.empty());
    global_instances.pop();
  }

  std::shared_ptr<ModuleInstance> peekBorrowedRuntimeInstance() {
    BOOST_ASSERT(!global_instances.empty());
    return global_instances.top();
  }

  std::shared_ptr<host_api::HostApi> peekHostApi() {
    return peekBorrowedRuntimeInstance()->getEnvironment().host_api;
  }

  template <typename T>
  WAVM::IR::ValueType get_wavm_type() = delete;

  template <>
  WAVM::IR::ValueType get_wavm_type<int32_t>() {
    return WAVM::IR::ValueType::i32;
  }

  template <>
  WAVM::IR::ValueType get_wavm_type<int64_t>() {
    return WAVM::IR::ValueType::i64;
  }

  template <auto Method, typename... Args>
  auto host_method_thunk(WAVM::Runtime::ContextRuntimeData *, Args... args) {
    return std::invoke(Method, peekHostApi(), args...);
  }

  template <auto Method, typename Ret, typename... Args>
  void registerMethod(IntrinsicModule &module, std::string_view name) {
    if constexpr (std::is_void_v<Ret>) {
      module.addFunction(
          name,
          host_method_thunk<Method, Args...>,
          WAVM::IR::FunctionType{{}, {get_wavm_type<Args>()...}});
    } else {
      module.addFunction(name,
                         host_method_thunk<Method, Args...>,
                         WAVM::IR::FunctionType{{get_wavm_type<Ret>()},
                                                {get_wavm_type<Args>()...}});
    }
  }

  void registerHostApiMethods(IntrinsicModule &module) {
    if (logger == nullptr) {
      logger = log::createLogger("Host API wrappers", "wavm");
    }
#define REGISTER_HOST_METHOD(Ret, name, ...)                                \
  registerMethod<&host_api::HostApi::name, Ret __VA_OPT__(, ) __VA_ARGS__>( \
      module, #name);

    REGISTER_HOST_METHODS
  }

}  // namespace kagome::runtime::wavm
