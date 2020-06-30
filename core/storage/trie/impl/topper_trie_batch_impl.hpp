/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL
#define KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL

#include "storage/trie/trie_batches.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"

namespace kagome::storage::trie {

  class TopperTrieBatchImpl : public TopperTrieBatch {
   public:
    enum class Error { PARENT_EXPIRED = 1 };

    explicit TopperTrieBatchImpl(std::shared_ptr<TrieBatch> parent);

    outcome::result<Buffer> get(const Buffer &key) const override;

    bool contains(const Buffer &key) const override;

    bool empty() const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;

    outcome::result<void> put(const Buffer &key, Buffer &&value) override;

    outcome::result<void> remove(const Buffer &key) override;

    outcome::result<void> clearPrefix(const Buffer &prefix) override;

    outcome::result<void> writeBack() override;

   private:
    std::map<Buffer, boost::optional<Buffer>> cache_;
    std::weak_ptr<TrieBatch> parent_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, TopperTrieBatchImpl::Error);

#endif  // KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL
