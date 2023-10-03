/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_header_repository.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"
#include "utils/lru_encoded.hpp"
#include "utils/safe_object.hpp"

namespace kagome::runtime {
  constexpr auto DISABLE_RUNTIME_LRU = false;

  /**
   * Cache runtime calls without arguments.
   */
  template <typename V>
  class RuntimeApiLruBlock {
   public:
    RuntimeApiLruBlock(size_t capacity) : lru_{capacity} {}

    outcome::result<std::shared_ptr<V>> call(Executor &executor,
                                             const primitives::BlockHash &block,
                                             std::string_view name) {
      if constexpr (DISABLE_RUNTIME_LRU) {
        OUTCOME_TRY(ctx, executor.ctx().ephemeralAt(block));
        return executor.call<std::shared_ptr<V>>(ctx, name);
      }
      if (auto r =
              lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
                return lru.get(block);
              })) {
        return *r;
      }
      OUTCOME_TRY(ctx, executor.ctx().ephemeralAt(block));
      OUTCOME_TRY(raw, ctx.module_instance->callExportFunction(ctx, name, {}));
      OUTCOME_TRY(r, ModuleInstance::decodedCall<V>(raw));
      return lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
        return lru.put(block, std::move(r), raw);
      });
    }

    void erase(const std::vector<primitives::BlockHash> &blocks) {
      if constexpr (DISABLE_RUNTIME_LRU) {
        return;
      }
      lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
        for (auto &block : blocks) {
          lru.erase(block);
        }
      });
    }

   private:
    SafeObject<LruEncoded<primitives::BlockHash, V>> lru_;
  };

  template <typename Arg>
  struct RuntimeApiLruBlockArgKey : std::pair<primitives::BlockHash, Arg> {};

  /**
   * Cache runtime calls with arguments.
   */
  template <typename Arg, typename V>
  class RuntimeApiLruBlockArg {
   public:
    using Key = RuntimeApiLruBlockArgKey<Arg>;

    RuntimeApiLruBlockArg(size_t capacity) : lru_{capacity} {}

    outcome::result<std::shared_ptr<V>> call(Executor &executor,
                                             const primitives::BlockHash &block,
                                             std::string_view name,
                                             const Arg &arg) {
      if constexpr (DISABLE_RUNTIME_LRU) {
        OUTCOME_TRY(ctx, executor.ctx().ephemeralAt(block));
        return executor.call<std::shared_ptr<V>>(ctx, name, arg);
      }
      Key key{{block, arg}};
      if (auto r =
              lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
                return lru.get(key);
              })) {
        return *r;
      }
      OUTCOME_TRY(ctx, executor.ctx().ephemeralAt(block));

      OUTCOME_TRY(raw_arg, ModuleInstance::encodeArgs(arg));
      OUTCOME_TRY(raw,
                  ctx.module_instance->callExportFunction(ctx, name, raw_arg));
      OUTCOME_TRY(r, ModuleInstance::decodedCall<V>(raw));
      return lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
        return lru.put(key, std::move(r), raw);
      });
    }

    void erase(const std::vector<primitives::BlockHash> &blocks) {
      if constexpr (DISABLE_RUNTIME_LRU) {
        return;
      }
      lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
        lru.erase_if([&](const Key &key, const std::shared_ptr<V> &) {
          return std::find_if(blocks.begin(),
                              blocks.end(),
                              [&](const primitives::BlockHash &block) {
                                return block == std::get<0>(key);
                              })
              != blocks.end();
        });
      });
    }

   private:
    SafeObject<LruEncoded<Key, V>> lru_;
  };

  /**
   * Only code upgrade changes `Core_version` and `Metadata_metadata` results.
   */
  template <typename V>
  class RuntimeApiLruCode {
   public:
    RuntimeApiLruCode(size_t capacity) : lru_{capacity} {}

    outcome::result<V> call(
        const blockchain::BlockHeaderRepository &block_header_repository,
        RuntimeUpgradeTracker &upgrades,
        Executor &executor,
        const primitives::BlockHash &block_hash,
        std::string_view name) {
      if constexpr (DISABLE_RUNTIME_LRU) {
        OUTCOME_TRY(ctx, executor.ctx().ephemeralAt(block_hash));
        return executor.call<V>(ctx, name);
      }
      OUTCOME_TRY(block_number,
                  block_header_repository.getNumberByHash(block_hash));
      OUTCOME_TRY(hash,
                  upgrades.getLastCodeUpdateState({block_number, block_hash}));
      if (auto r =
              lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
                auto v = lru.get(hash);
                return v ? std::make_optional(v->get()) : std::nullopt;
              })) {
        return *r;
      }
      OUTCOME_TRY(ctx, executor.ctx().ephemeralAt(block_hash));
      OUTCOME_TRY(r, executor.call<V>(ctx, name));
      return lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru) {
        return lru.put(hash, std::move(r));
      });
    }

   private:
    SafeObject<Lru<common::Hash256, V>> lru_;
  };
}  // namespace kagome::runtime

template <typename Arg>
struct std::hash<kagome::runtime::RuntimeApiLruBlockArgKey<Arg>> {
  size_t operator()(
      const kagome::runtime::RuntimeApiLruBlockArgKey<Arg> &x) const {
    return boost::hash_value(std::tie(x.first, x.second));
  }
};
