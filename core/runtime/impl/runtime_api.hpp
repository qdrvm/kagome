/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_API_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_API_HPP

#include "common/buffer.hpp"
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/wasm_memory.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime {

  struct ExecutorInterface {
    virtual ~ExecutorInterface() = default;

    virtual common::Buffer &state_code() = 0;
    virtual std::shared_ptr<WasmMemory> memory() = 0;
    virtual WasmExecutor &executor() = 0;
  };

  template <class R>
  struct TypedExecutor {
    explicit TypedExecutor(ExecutorInterface *exec) : exec_{exec} {}

    template <class... Args>
    outcome::result<R> execute(std::string_view name, Args &&... args) {
      runtime::WasmPointer ptr = 0u;
      runtime::SizeType len = 0u;

      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(buffer, scale::encode(std::forward<Args...>(args...)));
        len = buffer.size();
        ptr = exec_->memory()->allocate(len);
        exec_->memory()->storeBuffer(ptr, common::Buffer(std::move(buffer)));
      }

      wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(len)};
      wasm::Name wasm_name = std::string(name);
      OUTCOME_TRY(res,
                  exec_->executor().call(exec_->state_code(), wasm_name, ll));

      runtime::WasmPointer res_addr = getWasmAddr(res.geti64());
      runtime::SizeType res_len = getWasmLen(res.geti64());
      auto buffer = exec_->memory()->loadN(res_addr, res_len);

      return scale::decode<R>(buffer);
    };

    ExecutorInterface *exec_;
  };

  template <>
  struct TypedExecutor<void> {
    explicit TypedExecutor(ExecutorInterface *exec) : exec_{exec} {}

    template <class... Args>
    outcome::result<void> execute(std::string_view name, Args &&... args) {
      runtime::WasmPointer ptr = 0u;
      runtime::SizeType len = 0u;

      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(buffer, scale::encode(std::forward<Args...>(args...)));
        len = buffer.size();
        ptr = exec_->memory()->allocate(len);
        exec_->memory()->storeBuffer(ptr, common::Buffer(std::move(buffer)));
      }

      wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(len)};
      wasm::Name wasm_name = std::string(name);
      OUTCOME_TRY(exec_->executor().call(exec_->state_code(), wasm_name, ll));

      return outcome::success();
    };

    ExecutorInterface *exec_;
  };

  class RuntimeApi : public ExecutorInterface {
   public:
    ~RuntimeApi() override = default;

    common::Buffer &state_code() override {
      return state_code_;
    }

    std::shared_ptr<WasmMemory> memory() override {
      return memory_;
    }

    WasmExecutor &executor() override {
      return executor_;
    };

    RuntimeApi(common::Buffer state_code,
                    std::shared_ptr<extensions::Extension> extension)
        : state_code_(std::move(state_code)),
          memory_(extension->memory()),
          executor_(std::move(extension)) {}

    template <typename R, typename... Arg>
    outcome::result<R> execute(std::string_view name, Arg &&... args) {
      return TypedExecutor<R>(this).execute(name, std::forward<Arg...>(args...));
    }

   private:
    common::Buffer state_code_;
    std::shared_ptr<WasmMemory> memory_;
    WasmExecutor executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_API_HPP
