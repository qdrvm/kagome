/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_EXECUTOR_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_properties_cache.hpp"

#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define likely(x) __builtin_expect((x), 1)
#endif
#endif
#ifndef likely
#define likely(x) (x)
#endif

namespace kagome::runtime {

  class Executor {
   public:
    Executor(std::shared_ptr<RuntimeContextFactory> ctx_factory,
             std::optional<std::shared_ptr<RuntimePropertiesCache>> cache);

    const RuntimeContextFactory &ctx() const {
      return *ctx_factory_;
    }

    template <typename Res, typename... Args>
    outcome::result<Res> call(RuntimeContext &ctx,
                              std::string_view name,
                              const Args &...args) {
      auto code_hash = ctx.module_instance->getCodeHash();
      auto call = [&]() {
        return ctx.module_instance->template callAndDecodeExportFunction<Res>(
            ctx, name, args...);
      };
      if (!cache_) {
        return call();
      }
      auto &cache = *cache_;
      if constexpr (std::is_same_v<Res, primitives::Version>) {
        if (likely(name == "Core_version")) {
          return cache->getVersion(code_hash, call);
        }
      }
      if constexpr (std::is_same_v<Res, primitives::OpaqueMetadata>) {
        if (likely(name == "Metadata_metadata")) {
          return cache->getMetadata(code_hash, call);
        }
      }
      return call();
    }

    std::optional<std::shared_ptr<RuntimePropertiesCache>> cache_;
    std::shared_ptr<const RuntimeContextFactory> ctx_factory_;
  };

  //
  //  class Executor {
  //   public:
  //    using Buffer = common::Buffer;
  //    using BufferView = common::BufferView;
  //    using BlockHash = primitives::BlockHash;
  //
  //    explicit Executor(
  //        std::shared_ptr<RuntimeContextFactory> ctx_factory,
  //        std::optional<std::shared_ptr<RuntimePropertiesCache>> cache);
  //
  //    virtual ~Executor() = default;
  //
  //    struct Context {
  //      RuntimeContext &ctx;
  //    };
  //
  //    struct Ephemeral {
  //      outcome::result<RuntimeContext> create_ctx(
  //          const RuntimeContextFactory &ctx_factory) {
  //        if (root_hash) {
  //          return ctx_factory.ephemeralAt(block_hash, root_hash);
  //        } else {
  //          return ctx_factory.ephemeralAt(block_hash);
  //        }
  //      }
  //
  //      const primitives::BlockHash &block_hash;
  //      std::optional<storage::trie::RootHash> root_hash;
  //    };
  //
  //    struct Genesis {
  //      outcome::result<RuntimeContext> create_ctx(
  //          const RuntimeContextFactory &ctx_factory) {
  //        return ctx_factory.ephemeralAtGenesis();
  //      }
  //    };
  //
  //    using Config = std::variant<>;
  //
  //    outcome::result<Buffer> call(std::string_view name,
  //                                 BufferView encoded_args) {}
  //
  //    static outcome::result<Buffer> callWithCtx(RuntimeContext &ctx,
  //                                               std::string_view name,
  //                                               BufferView encoded_args);
  //
  //    outcome::result<Buffer> callAt(const primitives::BlockHash &block_hash,
  //                                   std::string_view name,
  //                                   BufferView encoded_args) {
  //      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash));
  //      return callWithCtx(ctx, name, encoded_args);
  //    }
  //
  //    outcome::result<Buffer> callAt(const primitives::BlockHash &block_hash,
  //                                   const storage::trie::RootHash
  //                                   &state_hash, std::string_view name,
  //                                   BufferView encoded_args) {
  //      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash, state_hash));
  //      return callWithCtx(ctx, name, encoded_args);
  //    }
  //
  //    outcome::result<Buffer> callAtGenesis(std::string_view name,
  //                                          BufferView encoded_args) {
  //      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAtGenesis());
  //      return callWithCtx(ctx, name, encoded_args);
  //    }
  //
  //    struct ArgStart {};
  //
  //    template <typename Call,
  //              typename Result,
  //              typename... CallArgs,
  //              typename... Args>
  //    static outcome::result<Result> decodedStatic(const Call &call,
  //                                                 CallArgs &&...call_args,
  //                                                 ArgStart,
  //                                                 Args &&...args) {
  //      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
  //      auto result_buf =
  //          call(std::forward<CallArgs>(call_args)..., encoded_args);
  //      return decodeResult<Result>(std::move(result_buf));
  //    }
  //
  //    template <typename Call,
  //              typename Result,
  //              typename... CallArgs,
  //              typename... Args>
  //    outcome::result<Result> decoded(const Call &call,
  //                                    CallArgs &&...call_args,
  //                                    ArgStart,
  //                                    Args &&...args) {
  //      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
  //      return cachedCall<Result>(
  //          ctx.module_instance->getCodeHash(),
  //          name,
  //          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
  //            auto result_buf = std::invoke(
  //                call, this, std::forward<CallArgs>(call_args)...,
  //                encoded_args);
  //            return decodeResult<Result>(std::move(result_buf));
  //          });
  //    }
  //
  //    /**
  //     * Method for calling a Runtime API method
  //     * Resets the runtime memory with the module's heap base,
  //     * encodes the arguments with SCALE codec, calls the method from the
  //     * provided module instance and  returns a result, decoded from SCALE.
  //     * Changes, made to the Host API state, are reset after the call.
  //     */
  //    template <typename Result, typename... Args>
  //    outcome::result<Result> decodedCallWithCtx(RuntimeContext &ctx,
  //                                               std::string_view name,
  //                                               Args &&...args) {
  //      return decoded(
  //          callWithCtx, ctx, name, ArgStart{}, std::forward<Args>(args)...);
  //      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
  //      return cachedCall<Result>(
  //          ctx.module_instance->getCodeHash(),
  //          name,
  //          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
  //            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
  //            return decodeResult<Result>(std::move(result_buf));
  //          });
  //    }
  //
  //    template <typename Result, typename... Args>
  //    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
  //                                   std::string_view name,
  //                                   Args &&...args) {
  //      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
  //      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash));
  //      return cachedCall<Result>(
  //          ctx.module_instance->getCodeHash(),
  //          name,
  //          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
  //            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
  //            return decodeResult<Result>(std::move(result_buf));
  //          });
  //    }
  //
  //    template <typename Result, typename... Args>
  //    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
  //                                   const storage::trie::RootHash
  //                                   &state_hash, std::string_view name, Args
  //                                   &&...args) {
  //      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
  //      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAt(block_hash, state_hash));
  //      return cachedCall<Result>(
  //          ctx.module_instance->getCodeHash(),
  //          name,
  //          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
  //            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
  //            return decodeResult<Result>(std::move(result_buf));
  //          });
  //    }
  //
  //    template <typename Result, typename... Args>
  //    outcome::result<Result> callAtGenesis(std::string_view name,
  //                                          Args &&...args) {
  //      OUTCOME_TRY(encoded_args, encodeArgs(std::forward<Args>(args)...));
  //      OUTCOME_TRY(ctx, ctx_factory_->ephemeralAtGenesis());
  //      return cachedCall<Result>(
  //          ctx.module_instance->getCodeHash(),
  //          name,
  //          [this, &ctx, name, &encoded_args]() -> outcome::result<Result> {
  //            OUTCOME_TRY(result_buf, callWithCtx(ctx, name, encoded_args));
  //            return decodeResult<Result>(std::move(result_buf));
  //          });
  //    }
  //
  //   private:
  //   private:
  //    template <typename Res, typename F>
  //    outcome::result<Res> cachedCall(const common::Hash256 &code_hash,
  //                                    std::string_view name,
  //                                    const F &call) {
  //      if (!cache_) {
  //        return call();
  //      }
  //      if constexpr (std::is_same_v<Res, primitives::Version>) {
  //        if (likely(name == "Core_version")) {
  //          return (*cache_)->getVersion(code_hash, call);
  //        }
  //      }
  //      if constexpr (std::is_same_v<Res, primitives::OpaqueMetadata>) {
  //        if (likely(name == "Metadata_metadata")) {
  //          return (*cache_)->getMetadata(code_hash, call);
  //        }
  //      }
  //      return call();
  //    }
  //
  //    std::shared_ptr<RuntimeContextFactory> ctx_factory_;
  //  };

}  // namespace kagome::runtime

#undef likely

#endif  // KAGOME_CORE_RUNTIME_RAW_EXECUTOR_HPP
