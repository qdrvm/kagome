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
      if (auto r =
              lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
                return lru_.get(block);
              })) {
        return *r;
      }
      OUTCOME_TRY(raw, executor.callAt(block, name, {}));
      OUTCOME_TRY(r, Executor::decodeResult<V>(raw));
      return lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
        return lru_.put(block, std::move(r), raw);
      });
    }

    void erase(const std::vector<primitives::BlockHash> &blocks) {
      lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
        for (auto &block : blocks) {
          lru_.erase(block);
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
      Key key{{block, arg}};
      if (auto r =
              lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
                return lru_.get(key);
              })) {
        return *r;
      }
      OUTCOME_TRY(raw_arg, Executor::encodeArgs(arg));
      OUTCOME_TRY(raw, executor.callAt(block, name, raw_arg));
      OUTCOME_TRY(r, Executor::decodeResult<V>(raw));
      return lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
        return lru_.put(key, std::move(r), raw);
      });
    }

    void erase(const std::vector<primitives::BlockHash> &blocks) {
      lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
        lru_.erase_if([&](const Key &key, const std::shared_ptr<V> &) {
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
      OUTCOME_TRY(block_number,
                  block_header_repository.getNumberByHash(block_hash));
      OUTCOME_TRY(hash,
                  upgrades.getLastCodeUpdateState({block_number, block_hash}));
      if (auto r =
              lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
                auto v = lru_.get(hash);
                return v ? std::make_optional(v->get()) : std::nullopt;
              })) {
        return *r;
      }
      OUTCOME_TRY(r, executor.callAt<V>(block_hash, name));
      return lru_.exclusiveAccess([&](typename decltype(lru_)::Type &lru_) {
        return lru_.put(hash, std::move(r));
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
