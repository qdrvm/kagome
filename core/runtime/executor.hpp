/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_context.hpp"
#include "log/profiling_logger.hpp"

namespace kagome::runtime {

  class Executor {
   public:
    using Buffer = common::Buffer;
    using BlockHash = primitives::BlockHash;
    virtual ~Executor() = default;

    /**
     * Call a runtime method \param name at state on block \param block_hash in
     * an ephemeral environment, e. g. the storage changes, made by this call,
     * will NOT persist in the node's Trie storage The call will be done with
     * the runtime code from \param block_info state Arguments for the call are
     * expected to be scale-encoded into single buffer \param encoded_args
     * beforehand \returns scale-encoded result
     */
    virtual outcome::result<Buffer> callAt(const BlockHash &block_hash,
                                           std::string_view name,
                                           const Buffer &encoded_args) = 0;

    virtual outcome::result<Buffer> callAtGenesis(
        std::string_view name, const Buffer &encoded_args) = 0;

    virtual outcome::result<Buffer> callWithCtx(RuntimeContext &ctx,
                                                std::string_view name,
                                                const Buffer &encoded_args) = 0;

    /**
     * Internal method for calling a Runtime API method
     * Resets the runtime memory with the module's heap base,
     * encodes the arguments with SCALE codec, calls the method from the
     * provided module instance and  returns a result, decoded from SCALE.
     * Changes, made to the Host API state, are reset after the call.
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callWithCtx(RuntimeContext &context,
                                        std::string_view name,
                                        Args &&...args) {
      common::Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }

      KAGOME_PROFILE_START(call_execution)

      OUTCOME_TRY(result_buf, callWithCtx(context, name, encoded_args));

      KAGOME_PROFILE_END(call_execution)

      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        Result t{};
        scale::ScaleDecoderStream s(result_buf);
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
