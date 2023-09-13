/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_properties_cache.hpp"

namespace kagome::runtime {

  class Executor {
   public:
    using Buffer = common::Buffer;
    using BufferView = common::BufferView;
    using BlockHash = primitives::BlockHash;

    explicit Executor(std::shared_ptr<RuntimeContextFactory> ctx_factory,
                      std::shared_ptr<RuntimePropertiesCache> cache);

    virtual ~Executor() = default;

    virtual outcome::result<Buffer> callWithCtx(RuntimeContext &ctx,
                                                std::string_view name,
                                                BufferView encoded_args);

    outcome::result<Buffer> callAt(const primitives::BlockHash &block_hash,
                                   std::string_view name,
                                   BufferView encoded_args) {
      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash));
      return callWithCtx(ctx, name, encoded_args);
    }

    outcome::result<Buffer> callAt(const primitives::BlockHash &block_hash,
                                   const storage::trie::RootHash &state_hash,
                                   std::string_view name,
                                   BufferView encoded_args) {
      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash, state_hash));
      return callWithCtx(ctx, name, encoded_args);
    }

    outcome::result<Buffer> callAtGenesis(std::string_view name,
                                          BufferView encoded_args) {
      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAtGenesis());
      return callWithCtx(ctx, name, encoded_args);
    }

    /**
     * Method for calling a Runtime API method
     * Resets the runtime memory with the module's heap base,
     * encodes the arguments with SCALE codec, calls the method from the
     * provided module instance and  returns a result, decoded from SCALE.
     * Changes, made to the Host API state, are reset after the call.
     */
    template <typename Result, typename... Args>
    outcome::result<Result> decodedCallWithCtx(RuntimeContext &ctx,
                                               std::string_view name,
                                               Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      return cachedCall<Result>(
          ctx.module_instance->getCodeHash(),
          name,
          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
            return decodeResult<Result>(std::move(result_buf));
          });
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash));
      return cachedCall<Result>(
          ctx.module_instance->getCodeHash(),
          name,
          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
            return decodeResult<Result>(std::move(result_buf));
          });
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
                                   const storage::trie::RootHash &state_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash, state_hash));
      return cachedCall<Result>(
          ctx.module_instance->getCodeHash(),
          name,
          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
            return decodeResult<Result>(std::move(result_buf));
          });
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAtGenesis(std::string_view name,
                                          Args &&...args) {
      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAtGenesis());
      return cachedCall<Result>(
          ctx.module_instance->getCodeHash(),
          name,
          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
            return decodeResult<Result>(std::move(result_buf));
          });
    }

    template <typename... Args>
    static outcome::result<common::Buffer> encodeArgs(Args &&...args) {
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        return Buffer{std::move(res)};
      }
      return Buffer{};
    }

    template <typename Result>
    static outcome::result<Result> decodeResult(common::BufferView result) {
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

   private:
    template <typename Res, typename F>
    outcome::result<Res> cachedCall(const common::Hash256 &code_hash,
                                    std::string_view name,
                                    const F &call) {
      if constexpr (std::is_same_v<Res, primitives::Version>) {
        likely_if(name == "Core_version") {
          return cache_->getVersion(code_hash, call);
        }
      }
      if constexpr (std::is_same_v<Res, primitives::OpaqueMetadata>) {
        likely_if(name == "Metadata_metadata") {
          return cache_->getMetadata(code_hash, call);
        }
      }
      return call();
    }

    std::shared_ptr<RuntimeContextFactory> ctx_factory_;
    std::shared_ptr<RuntimePropertiesCache> cache_;
  };

}  // namespace kagome::runtime
