/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_API_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_API_HPP

#include <utility>

#include "common/buffer.hpp"
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/wasm_memory.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime {
  /**
   * @brief base class for all runtime apis
   */
  class RuntimeApi {
   public:
    RuntimeApi(common::Buffer state_code,
               std::shared_ptr<extensions::Extension> extension)
        : state_code_(std::move(state_code)),
          memory_(extension->memory()),
          executor_(std::move(extension)) {}

    /**
     * @brief executes wasm export method returning non-void result
     * @tparam R result type including void
     * @tparam Args arguments types list
     * @param name export method name
     * @param args arguments
     * @return parsed result or error
     */
    template <typename R,  // typename = std::enable_if_t<!std::is_same_v<R,
                           // void>>,
              typename... Args>
    outcome::result<R> execute(std::string_view name, Args &&... args) {
      runtime::WasmPointer ptr = 0u;
      runtime::SizeType len = 0u;

      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(buffer, scale::encode(std::forward<Args>(args)...));
        len = buffer.size();
        ptr = memory_->allocate(len);
        memory_->storeBuffer(ptr, common::Buffer(std::move(buffer)));
      }

      wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(len)};
      wasm::Name wasm_name = std::string(name);
      OUTCOME_TRY(res, executor_.call(state_code_, wasm_name, ll));

      if constexpr (std::is_same<void, R>::value) {
        return outcome::success();
      } else {
        WasmResult r{res.geti64()};
        // TODO (yuraz) PRE-98: after check for memory overflow is done,
        //  refactor it
        auto buffer = memory_->loadN(r.address(), r.length());

        return scale::decode<R>(buffer);
      }
    }

   private:
    common::Buffer state_code_;
    std::shared_ptr<WasmMemory> memory_;
    WasmExecutor executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_API_HPP
