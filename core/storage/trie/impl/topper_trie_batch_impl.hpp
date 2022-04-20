/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL
#define KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL

#include "storage/trie/trie_batches.hpp"

#include <deque>

#include "outcome/outcome.hpp"

namespace kagome::storage::trie {
  class PolkadotTrieCursor;
}

namespace kagome::storage::trie {

  class TopperTrieBatchImpl final : public TopperTrieBatch {
   public:
    enum class Error { PARENT_EXPIRED = 1 };

    explicit TopperTrieBatchImpl(const std::shared_ptr<TrieBatch> &parent);

    outcome::result<common::BufferConstRef> get(
        const BufferView &key) const override;
    outcome::result<std::optional<common::BufferConstRef>> tryGet(
        const BufferView &key) const override;

    /**
     * Won't consider changes not written back to the parent batch
     */
    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;
    outcome::result<bool> contains(const BufferView &key) const override;
    bool empty() const override;

    outcome::result<RootHash> calculateRoot() const override {
      OUTCOME_TRY(const_cast<TopperTrieBatchImpl*>(this)->writeBack());
      return parent_.lock()->calculateRoot();
    }

    outcome::result<void> put(const BufferView &key,
                              const Buffer &value) override;
    outcome::result<void> put(const BufferView &key, Buffer &&value) override;
    outcome::result<void> remove(const BufferView &key) override;
    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const BufferView &prefix, std::optional<uint64_t> limit) override;

    outcome::result<void> writeBack() override;

   private:
    bool wasClearedByPrefix(const BufferView &key) const;

    std::map<Buffer, std::optional<Buffer>, std::less<>> cache_;
    std::deque<Buffer> cleared_prefixes_;
    std::weak_ptr<TrieBatch> parent_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, TopperTrieBatchImpl::Error)

#endif  // KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL
