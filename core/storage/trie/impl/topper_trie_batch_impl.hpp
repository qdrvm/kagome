/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/trie_batches.hpp"

#include <deque>

#include "outcome/outcome.hpp"

namespace kagome::storage::trie {
  class TopperTrieBatchImpl final
      : public TrieBatch,
        public std::enable_shared_from_this<TopperTrieBatchImpl> {
   public:
    enum class Error : uint8_t {
      PARENT_EXPIRED = 1,
      CHILD_BATCH_NOT_SUPPORTED,
      COMMIT_NOT_SUPPORTED,
      CURSOR_NEXT_INVALID,
      CURSOR_SEEK_LAST_NOT_IMPLEMENTED,
      CURSOR_PREV_NOT_IMPLEMENTED,
    };

    explicit TopperTrieBatchImpl(const std::shared_ptr<TrieBatch> &parent);

    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;

    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;
    outcome::result<bool> contains(const BufferView &key) const override;

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;
    outcome::result<void> remove(const BufferView &key) override;
    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const BufferView &prefix, std::optional<uint64_t> limit) override;

    outcome::result<void> writeBack();

    outcome::result<RootHash> commit(StateVersion version) override;

    outcome::result<std::optional<std::shared_ptr<TrieBatch>>> createChildBatch(
        common::BufferView path) override;

    outcome::result<void> apply(storage::BufferStorage &map);

   private:
    std::map<Buffer, std::optional<Buffer>> cache_;
    std::weak_ptr<TrieBatch> parent_;

    friend class TopperTrieCursor;
  };

  /**
   * Required for:
   * - ext_storage_next_key_version_1
   * - ext_default_child_storage_next_key_version_1
   */
  class TopperTrieCursor : public PolkadotTrieCursor {
   public:
    TopperTrieCursor(std::shared_ptr<TopperTrieBatchImpl> batch,
                     std::unique_ptr<PolkadotTrieCursor> cursor);

    outcome::result<bool> seekFirst() override;
    outcome::result<bool> seek(const BufferView &key) override;
    outcome::result<bool> seekLast() override;
    bool isValid() const override;
    outcome::result<void> next() override;
    outcome::result<void> prev() override;
    std::optional<Buffer> key() const override;
    std::optional<BufferOrView> value() const override;

    outcome::result<void> seekLowerBound(const BufferView &key) override;
    outcome::result<void> seekUpperBound(const BufferView &key) override;

   private:
    void updateSource();
    bool isRemoved() const;
    outcome::result<void> skipRemoved();
    outcome::result<void> step();

    // since topper batches accumulate changes in an associative cache,
    // sometimes cursor needs to switch between this cache and the actual trie
    // beneath
    struct Source {
      Source(bool parent, bool overlay) : parent{parent}, overlay{overlay} {}

      bool hasValidSource() const {
        return parent || overlay;
      }

      explicit operator bool() const {
        return hasValidSource();
      }

      bool parent;   // parent_cursor_ is valid
      bool overlay;  // overlay_it_ is valid
    };

    std::shared_ptr<TopperTrieBatchImpl> parent_batch_;
    std::unique_ptr<PolkadotTrieCursor> parent_cursor_;
    std::optional<Buffer> cached_parent_key_;
    decltype(TopperTrieBatchImpl::cache_)::iterator overlay_it_;
    Source source_{false, false};
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, TopperTrieBatchImpl::Error)
