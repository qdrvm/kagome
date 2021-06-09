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
#include "host_api/host_api_factory.hpp"
#include "log/logger.hpp"
#include "runtime/binaryen/runtime_environment.hpp"
#include "runtime/binaryen/runtime_environment_factory_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/binaryen/wasm_executor.hpp"
#include "runtime/memory.hpp"
#include "runtime/runtime_code_provider.hpp"
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
      EPHEMERAL,   // the changes made by this call will vanish once it's
                   // completed
      ISOLATED  // this call is executed in an isolated environment and must not
                // affect neither host storage nor runtime memory
    };

    RuntimeApi(std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory)
        : runtime_env_factory_(std::move(runtime_env_factory)) {
      BOOST_ASSERT(runtime_env_factory_);
    }

   protected:
    struct CallConfig {
      CallPersistency persistency;
      RuntimeEnvironmentFactory::Config runtime_env_config{};
    };

   private:
    // as it has a deduced return type, must be defined before execute()
    auto createRuntimeEnvironment(
        const CallConfig &config,
        const boost::optional<common::Hash256> &state_root_opt) {
      if (state_root_opt.has_value()) {
        switch (config.persistency) {
          case CallPersistency::PERSISTENT:
            return runtime_env_factory_
                ->makePersistentAt(state_root_opt.value())
                .value();
          case CallPersistency::EPHEMERAL:
            return runtime_env_factory_->makeEphemeralAt(state_root_opt.value())
                .value();
          case CallPersistency::ISOLATED:
            return runtime_env_factory_
                ->makeIsolatedAt(state_root_opt.value(),
                                 config.runtime_env_config)
                .value();
        }
      } else {
        switch (config.persistency) {
          case CallPersistency::PERSISTENT:
            return runtime_env_factory_->makePersistent().value();
          case CallPersistency::EPHEMERAL:
            return runtime_env_factory_->makeEphemeral().value();
          case CallPersistency::ISOLATED:
            return runtime_env_factory_->makeIsolated(config.runtime_env_config)
                .value();
        }
      }
      BOOST_UNREACHABLE_RETURN({});
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
     * persist in the state, EPHEMERAL if they can be discarded
     * @param args - export method arguments
     * @return a parsed result or an error
     */
    template <typename R, typename... Args>
    outcome::result<R> executeAt(std::string_view name,
                                 const storage::trie::RootHash &state_root,
                                 CallConfig config,
                                 Args &&... args) {
      return executeInternal<R>(
          name, state_root, std::move(config), std::forward<Args>(args)...);
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
                               CallConfig config,
                               Args &&... args) {
      return executeInternal<R>(
          name, boost::none, std::move(config), std::forward<Args>(args)...);
    }

   private:
    /**
     * If \arg state_root contains a value, then the state will be reset to the
     * hash in this value, otherwise the export method will be executed on the
     * current state
     * @note for explanation of arguments \see execute or \see executeAt
     */
    template <typename R, typename... Args>
    outcome::result<R> executeInternal(
        std::string_view name,
        boost::optional<storage::trie::RootHash> state_root,
        CallConfig config,
        Args &&... args) {
      SL_DEBUG(logger_, "Executing export function: {}", name);
      if (state_root.has_value()) {
        SL_DEBUG(logger_, "Resetting state to: {}", state_root.value().toHex());
      }

      auto &&[module_instance, memory, opt_batch] =
          createRuntimeEnvironment(config, state_root);

      gsl::final_action dispose(
          [memory = memory, module_instance = module_instance] {
            memory->reset();
            module_instance->reset();
          });

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

      OUTCOME_TRY(res, executor_.call(*module_instance, wasm_name, ll));

      if constexpr (!std::is_same_v<void, R>) {
        PtrSize r(res.geti64());
        auto buffer = memory->loadN(r.ptr, r.size);
        return scale::decode<R>(std::move(buffer));
      }

      if (opt_batch) {
        OUTCOME_TRY(opt_batch.value()->writeBack());
      }
      return outcome::success();
    }

    std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory_;
    WasmExecutor executor_;
    log::Logger logger_ = log::createLogger("RuntimeApi", "runtime");
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_HPP
