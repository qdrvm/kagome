/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP

#include <utility>

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>
#include <boost/optional.hpp>

#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "extensions/extension_factory.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/binaryen/runtime_manager.hpp"
#include "runtime/binaryen/wasm_executor.hpp"
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
    enum class CallPersistency {
      PERSISTENT,  // the changes made by this call will be applied to the state
                   // trie storage
      EPHEMERAL    // the changes made by this call will vanish once it's
                   // completed
    };

    RuntimeApi(std::shared_ptr<RuntimeManager> runtime_manager)
        : runtime_manager_(std::move(runtime_manager)) {
      BOOST_ASSERT(runtime_manager_);
    }

   private:
    // as it has a deduced return type, must be defined before execute()
    auto getRuntimeEnvironment(
        CallPersistency persistency,
        const boost::optional<common::Hash256> &state_root_opt) {
      if (state_root_opt.has_value()) {
        switch (persistency) {
          case CallPersistency::PERSISTENT:
            return runtime_manager_
                ->createPersistentRuntimeEnvironmentAt(
                    state_root_opt.value())
                .value();
          case CallPersistency::EPHEMERAL:
            return runtime_manager_
                ->createEphemeralRuntimeEnvironmentAt(state_root_opt.value())
                .value();
        }
      } else {
        switch (persistency) {
          case CallPersistency::PERSISTENT:
            return runtime_manager_->createPersistentRuntimeEnvironment().value();
          case CallPersistency::EPHEMERAL:
            return runtime_manager_->createEphemeralRuntimeEnvironment().value();
        }
      }
    }

   protected:
    /**
     * @brief executes wasm export method returning non-void result
     * @tparam R result type including void
     * @tparam Args arguments types list
     * @param name - export method name
     * @param state_root - a hash of the state root to which the state will be
     * reset before executing the export method
     * @param persistency - PERSISTENT if changes made by the method should
     * persist in the state, EPHEMERAL if they can be discraded
     * @param args - export method arguments
     * @return a parsed result or an error
     */
    template <typename R, typename... Args>
    outcome::result<R> executeAt(std::string_view name,
                                 const common::Hash256 &state_root,
                                 CallPersistency persistency,
                                 Args &&... args) {
      return executeMaybeAt<R>(
          name, state_root, persistency, std::forward<Args>(args)...);
    }

    /**
     * @brief executes wasm export method returning non-void result
     * @tparam R result type including void
     * @tparam Args arguments types list
     * @param name - export method name
     * @param persistency - PERSISTENT if changes made by the method should
     * persist in the state, EPHEMERAL if they can be discraded
     * @param args - export method arguments
     * @return a parsed result or an error
     */
    template <typename R, typename... Args>
    outcome::result<R> execute(std::string_view name,
                               CallPersistency persistency,
                               Args &&... args) {
      return executeMaybeAt<R>(
          name, boost::none, persistency, std::forward<Args>(args)...);
    }

   private:
    /**
     * If \arg state_root contains a value, then the state will be reset to the
     * hash in this value, otherwise the export method will be executed on the
     * current state
     * @note for explanation of arguments \see execute or \see executeAt
     */
    template <typename R, typename... Args>
    outcome::result<R> executeMaybeAt(
        std::string_view name,
        const boost::optional<common::Hash256> &state_root,
        CallPersistency persistency,
        Args &&... args) {
      logger_->debug("Executing export function: {}", name);
      if (state_root.has_value()) {
        logger_->debug("Resetting state to: {}", state_root.value().toHex());
      }

      auto environment = getRuntimeEnvironment(persistency, state_root);
      auto &&[module, memory] = environment;

      runtime::WasmPointer ptr = 0u;
      runtime::WasmSize len = 0u;

      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(buffer, scale::encode(std::forward<Args>(args)...));
        len = buffer.size();
        ptr = memory->allocate(len);
        memory->storeBuffer(ptr, common::Buffer(std::move(buffer)));
      }

      wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(len)};

      wasm::Name wasm_name = std::string(name);

      OUTCOME_TRY(res, executor_.call(*module, wasm_name, ll));
      memory->reset();
      if constexpr (!std::is_same_v<void, R>) {
        WasmResult r(res.geti64());
        auto buffer = memory->loadN(r.address, r.length);
        // TODO (yuraz) PRE-98: after check for memory overflow is done,
        //  refactor it
        return scale::decode<R>(std::move(buffer));
      }

      return outcome::success();
    }
    std::shared_ptr<RuntimeManager> runtime_manager_;
    WasmExecutor executor_;
    common::Logger logger_ = common::createLogger("Runtime API");
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP
