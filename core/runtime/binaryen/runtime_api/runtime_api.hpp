/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP

#include <utility>

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>

#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "extensions/extension_factory.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/binaryen/wasm_executor.hpp"
#include "runtime/binaryen/runtime_manager.hpp"
#include "runtime/wasm_memory.hpp"
#include "runtime/wasm_provider.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime::binaryen {

  /**
   * @brief base class for all runtime apis
   */
  class RuntimeApi {
   public:
    RuntimeApi(std::shared_ptr<RuntimeManager> runtime_manager)
        : runtime_manager_(std::move(runtime_manager)) {
      BOOST_ASSERT(runtime_manager_);
    }

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
      logger_->debug("Executing export function: {}", name);

      OUTCOME_TRY(environment, runtime_manager_->getRuntimeEvironment());
      auto &&[module, memory] = std::move(environment);

      runtime::WasmPointer ptr = 0u;
      runtime::SizeType len = 0u;

      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(buffer, scale::encode(std::forward<Args>(args)...));
        len = buffer.size();
        ptr = memory->allocate(len);
        memory->storeBuffer(ptr, common::Buffer(std::move(buffer)));
      }

      wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(len)};

      wasm::Name wasm_name = std::string(name);

      OUTCOME_TRY(res, executor_.call(*module, wasm_name, ll));

      if constexpr (!std::is_same_v<void, R>) {
        WasmResult r{res.geti64()};
        auto buffer = memory->loadN(r.address, r.length);
        // TODO (yuraz) PRE-98: after check for memory overflow is done,
        //  refactor it
        return scale::decode<R>(std::move(buffer));
      }

      return outcome::success();
    }

   private:
    std::shared_ptr<RuntimeManager> runtime_manager_;
    WasmExecutor executor_;
    common::Logger logger_ = common::createLogger("Runtime api");
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP
