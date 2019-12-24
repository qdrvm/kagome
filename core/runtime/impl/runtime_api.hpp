/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_API_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_API_HPP

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>

#include <utility>

#include "common/buffer.hpp"
#include "extensions/extension_factory.hpp"
#include "runtime/impl/runtime_external_interface.hpp"
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/wasm_memory.hpp"
#include "runtime/wasm_provider.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime {
  /**
   * @brief base class for all runtime apis
   */
  class RuntimeApi {
   public:
    RuntimeApi(std::shared_ptr<runtime::WasmProvider> wasm_provider,
               std::shared_ptr<extensions::ExtensionFactory> extension_factory)
        : wasm_provider_(std::move(wasm_provider)),
          extension_factory_(std::move(extension_factory)),
          executor_{} {}

   protected:
    /**
     * @brief executes wasm export method returning non-void result
     * @tparam R result type including void
     * @tparam Args arguments types list
     * @param name export method name
     * @param args arguments
     * @return parsed result or error
     */
    template <typename R, typename... Args>
    outcome::result<R> execute(std::string_view name, Args &&... args) {
      runtime::WasmPointer ptr = 0u;
      runtime::SizeType len = 0u;

      RuntimeExternalInterface rei{extension_factory_};

      wasm::Name wasm_name = std::string(name);
      const auto &state_code = wasm_provider_->getStateCode();
      OUTCOME_TRY(module, executor_.prepareModule(state_code));
      auto module_instance = executor_.prepareModuleInstance(module, rei);
      auto memory = rei.memory();

      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(buffer, scale::encode(std::forward<Args>(args)...));
        len = buffer.size();
        ptr = memory->allocate(len);
        memory->storeBuffer(ptr, common::Buffer(std::move(buffer)));
      }

      wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(len)};

      OUTCOME_TRY(res, executor_.call(module_instance, wasm_name, ll));

      if constexpr (!std::is_same_v<void, R>) {
        WasmResult r{res.geti64()};
        auto buffer = memory->loadN(r.address, r.length);
        // TODO (yuraz) PRE-98: after check for memory overflow is done,
        //  refactor it
        return scale::decode<R>(buffer);
      }

      return outcome::success();
    }

   private:
    std::shared_ptr<runtime::WasmProvider> wasm_provider_;
    std::shared_ptr<extensions::ExtensionFactory> extension_factory_;
    WasmExecutor executor_;
  };
}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_RUNTIME_API_HPP
