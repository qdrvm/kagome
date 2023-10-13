/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_tree.hpp"
#include "storage/buffer_map_types.hpp"
#include "utils/block_info_key.hpp"

namespace kagome::blockchain {
  /**
   * Cached ancestry check.
   */
  struct Descent {
    Descent(std::shared_ptr<blockchain::BlockTree> block_tree,
            primitives::BlockInfo start)
        : block_tree_{std::move(block_tree)}, path_{start} {}

    /**
     * Checks if `to` is ancestor of `start`.
     * Caches intermediate blocks if `update_path_` is true.
     */
    bool descends(const primitives::BlockInfo &to) const {
      if (to == path_.front()) {
        return true;
      }
      if (to.number >= path_.front().number) {
        return false;
      }
      auto i = indexFor(to.number);
      if (i >= path_.size()) {
        if (not update_path_) {
          return block_tree_->hasDirectChain(to, path_.back());
        }
        auto chain_res = block_tree_->getDescendingChainToBlock(
            path_.back().hash, path_.back().number - to.number + 1);
        if (not chain_res) {
          return false;
        }
        auto &chain = chain_res.value();
        if (chain.size() <= 1) {
          return false;
        }
        path_.reserve(path_.size() + chain.size() - 1);
        for (size_t j = 1; j < chain.size(); ++j) {
          path_.emplace_back(path_.back().number - 1, chain[j]);
        }
        if (i >= path_.size()) {
          return false;
        }
      }
      return to == path_.at(i);
    }

    /**
     * Get index in `path_` for block.
     */
    size_t indexFor(primitives::BlockNumber n) const {
      BOOST_ASSERT(n <= path_.front().number);
      return path_.front().number - n;
    }

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    mutable std::vector<primitives::BlockInfo> path_;
    bool update_path_ = true;
  };

  template <typename T>
  struct Indexed {
    SCALE_TIE_ONLY(value, prev);

    /**
     * Empty `value` means that blocks from `prev` to current have been indexed,
     * and current block doesn't have own `value`.
     */
    std::optional<T> value;
    /**
     * Previous block with value.
     */
    std::optional<primitives::BlockInfo> prev;
    /**
     * Does this block inherit value from `prev` or has own `value`.
     */
    bool inherit = false;
  };

  /**
   * Used to store and query inheritable values for blocks.
   * Used to store changes from digests.
   * Ensures that no block between found and requested change that value.
   */
  template <typename T>
  struct Indexer {
    Indexer(std::shared_ptr<storage::BufferStorage> db,
            std::shared_ptr<blockchain::BlockTree> block_tree)
        : db_{std::move(db)}, block_tree_{std::move(block_tree)} {
      primitives::BlockInfo genesis{0, block_tree_->getGenesisBlockHash()};
      last_finalized_indexed_ = genesis;
      map_.emplace(genesis, Indexed<T>{});
    }

    outcome::result<void> init() {
      auto batch = db_->batch();
      auto db_cur = db_->cursor();
      OUTCOME_TRY(db_cur->seekFirst());
      while (db_cur->isValid()) {
        auto info = BlockInfoKey::decode(*db_cur->key()).value();
        if (not block_tree_->isFinalized(info)) {
          OUTCOME_TRY(batch->remove(BlockInfoKey::encode(info)));
        } else {
          last_finalized_indexed_ = info;
          BOOST_OUTCOME_TRY(map_[info],
                            scale::decode<Indexed<T>>(*db_cur->value()));
        }
        OUTCOME_TRY(db_cur->next());
      }
      OUTCOME_TRY(batch->commit());
      return outcome::success();
    }

    Descent descend(const primitives::BlockInfo &from) const {
      return {block_tree_, from};
    }

    std::optional<Indexed<T>> get(const primitives::BlockInfo &block) const {
      if (auto it = map_.find(block); it != map_.end()) {
        return it->second;
      }
      if (auto r = db_->tryGet(BlockInfoKey::encode(block)).value()) {
        return scale::decode<Indexed<T>>(*r).value();
      }
      return std::nullopt;
    }

    void put(const primitives::BlockInfo &block,
             const Indexed<T> &indexed,
             bool db) {
      if (indexed.inherit and block.number <= last_finalized_indexed_.number) {
        return;
      }
      map_[block] = indexed;
      if (db) {
        db_->put(BlockInfoKey::encode(block), scale::encode(indexed).value())
            .value();
      }
    }

    void remove(const primitives::BlockInfo &block) {
      map_.erase(block);
      db_->remove(BlockInfoKey::encode(block)).value();
    }

    void finalize() {
      auto batch = db_->batch();
      auto finalized = block_tree_->getLastFinalized();
      auto first = last_finalized_indexed_.number + 1;
      for (auto map_it = map_.lower_bound({first, {}}); map_it != map_.end();) {
        auto &[info, indexed] = *map_it;
        if (block_tree_->isFinalized(info)) {
          if (not indexed.inherit) {
            batch
                ->put(BlockInfoKey::encode(info),
                      scale::encode(indexed).value())
                .value();
            last_finalized_indexed_ = info;
          }
        } else if (not block_tree_->hasDirectChain(finalized, info)) {
          map_it = map_.erase(map_it);
          continue;
        }
        ++map_it;
      }
      for (auto map_it = map_.lower_bound({first, {}});
           map_it != map_.end()
           and map_it->first.number < last_finalized_indexed_.number;) {
        if (map_it->second.inherit) {
          map_it = map_.erase(map_it);
        } else {
          ++map_it;
        }
      }
      batch->commit().value();
    }

    using KeyValue = std::pair<primitives::BlockInfo, Indexed<T>>;
    struct SearchRaw {
      KeyValue kv;
      primitives::BlockInfo last;
    };

    std::optional<SearchRaw> searchRaw(Descent &descent,
                                       const primitives::BlockInfo &block) {
      auto map_it = map_.lower_bound(block);
      while (true) {
        if (map_it != map_.end() and descent.descends(map_it->first)) {
          if (not map_it->second.inherit) {
            return SearchRaw{*map_it, map_it->first};
          }
          BOOST_ASSERT(map_it->second.prev);
          auto r = get(*map_it->second.prev);
          BOOST_ASSERT(r);
          return SearchRaw{
              {*map_it->second.prev, std::move(*r)},
              map_it->first,
          };
        }
        if (map_it == map_.begin()) {
          return std::nullopt;
        }
        --map_it;
      }
    }

    /**
     * Search first inherited value for `block` descending by `descent`.
     * Unindexed blocks are indexed with `cb`.
     * `cb` args:
     * - `Option<BlockInfo>` of previous block with value.
     * - `size_t` and `size_t` [first..last] indices into `descent.path_`
     *   blocks. `descent.path_` is reversed, so indices decrease.
     */
    template <typename Cb>
    std::optional<KeyValue> search(Descent &descent,
                                   const primitives::BlockInfo &block,
                                   const Cb &cb) {
      descent.update_path_ = true;
      auto raw = searchRaw(descent, block);
      if (not raw) {
        return std::nullopt;
      }
      BOOST_ASSERT(not raw->kv.second.inherit);
      if (not raw->kv.second.value
          or (raw->last != block
              and (block.number > last_finalized_indexed_.number
                   or not block_tree_->isFinalized(block)))) {
        auto prev = raw->kv.second.value ? raw->kv.first : raw->kv.second.prev;
        auto i_first =
            descent.indexFor(raw->last.number + (raw->kv.second.value ? 1 : 0));
        BOOST_ASSERT(i_first < descent.path_.size());
        auto i_last = descent.indexFor(block.number);
        BOOST_ASSERT(i_last < descent.path_.size());
        cb(prev, i_first, i_last);
        descent.update_path_ = false;
        raw = searchRaw(descent, block);
        if (not raw or not raw->kv.second.value or raw->last != block) {
          return std::nullopt;
        }
      }
      return raw->kv;
    }

    std::shared_ptr<storage::BufferStorage> db_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    primitives::BlockInfo last_finalized_indexed_;
    std::map<primitives::BlockInfo, Indexed<T>> map_;
  };
}  // namespace kagome::blockchain
