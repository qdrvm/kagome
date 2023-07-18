/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_EXECUTOR_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_context.hpp"

namespace kagome::runtime {

  class Executor {
   public:
    using Buffer = common::Buffer;
    using BlockHash = primitives::BlockHash;
    virtual ~Executor() = default;

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the \param block_info state
     */
    virtual outcome::result<std::unique_ptr<RuntimeContext>>
    getPersistentContextAt(
        const primitives::BlockHash &block_hash,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker) = 0;

    virtual outcome::result<std::unique_ptr<RuntimeContext>>
    getEphemeralContextAt(const primitives::BlockHash &block_hash) = 0;

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the code of \param block_info state at \param
     * state_hash state
     */
    virtual outcome::result<std::unique_ptr<RuntimeContext>>
    getEphemeralContextAt(const primitives::BlockHash &block_hash,
                          const storage::trie::RootHash &state_hash) = 0;

    virtual outcome::result<std::unique_ptr<RuntimeContext>>
    getEphemeralContextAtGenesis() = 0;

    virtual outcome::result<Buffer> callWithCtx(RuntimeContext &ctx,
                                                std::string_view name,
                                                const Buffer &encoded_args) = 0;

    outcome::result<Buffer> callAt(const primitives::BlockHash &block_hash,
                                   std::string_view name,
                                   const Buffer &encoded_args) {
      OUTCOME_TRY(ctx, getEphemeralContextAt(block_hash));
      return callWithCtx(*ctx, name, encoded_args);
    }

    outcome::result<Buffer> callAt(const primitives::BlockHash &block_hash,
                                   const storage::trie::RootHash &state_hash,
                                   std::string_view name,
                                   const Buffer &encoded_args) {
      OUTCOME_TRY(ctx, getEphemeralContextAt(block_hash, state_hash));
      return callWithCtx(*ctx, name, encoded_args);
    }

    outcome::result<Buffer> callAtGenesis(std::string_view name,
                                          const Buffer &encoded_args) {
      OUTCOME_TRY(ctx, getEphemeralContextAtGenesis());
      return callWithCtx(*ctx, name, encoded_args);
    }

    /**
     * Method for calling a Runtime API method
     * Resets the runtime memory with the module's heap base,
     * encodes the arguments with SCALE codec, calls the method from the
     * provided module instance and  returns a result, decoded from SCALE.
     * Changes, made to the Host API state, are reset after the call.
     */
    template <typename Result, typename... Args>
    outcome::result<Result> decodedCallWithCtx(RuntimeContext &context,
                                        std::string_view name,
                                        Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(result_buf, callWithCtx(context, name, encoded_args));

      return decodeResult<Result>(std::move(result_buf));
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(result_buf, callAt(block_hash, name, encoded_args));

      return decodeResult<Result>(std::move(result_buf));
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
                                   const storage::trie::RootHash &state_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(result_buf, callAt(block_hash, name, encoded_args));

      return decodeResult<Result>(std::move(result_buf));
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAtGenesis(std::string_view name,
                                          Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(result_buf, callAtGenesis(name, encoded_args));

      return decodeResult<Result>(std::move(result_buf));
    }

   private:
    template <typename... Args>
    static outcome::result<common::Buffer> encodeArgs(Args &&...args) {
      common::Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }
      return encoded_args;
    }

    template <typename Result>
    static outcome::result<Result> decodeResult(common::Buffer &&result) {
      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        Result t{};
        scale::ScaleDecoderStream s(result);
        try {
          s >> t;
          // Check whether the whole byte buffer was consumed
          if (s.hasMore(1)) {
            static auto logger = log::createLogger("Executor", "runtime");
            SL_ERROR(logger,
                     "Runtime API call result size exceeds the size of the "
                     "type to initialize {} (read {}, total size {})",
                     typeid(Result).name(),
                     s.currentIndex(),
                     s.span().size_bytes());
            return outcome::failure(std::errc::illegal_byte_sequence);
          }
          return outcome::success(std::move(t));
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      }
    }
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP
