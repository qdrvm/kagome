/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_batch_base.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

namespace kagome::storage::trie {

  TrieBatchBase::TrieBatchBase(std::shared_ptr<Codec> codec,
                               std::shared_ptr<TrieSerializer> serializer,
                               std::shared_ptr<PolkadotTrie> trie)
      : codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        trie_{std::move(trie)} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(trie_ != nullptr);
  }

  outcome::result<BufferOrView> TrieBatchBase::get(
      const BufferView &key) const {
    return trie_->get(key);
  }

  outcome::result<std::optional<BufferOrView>> TrieBatchBase::tryGet(
      const BufferView &key) const {
    return trie_->tryGet(key);
  }

  std::unique_ptr<PolkadotTrieCursor> TrieBatchBase::trieCursor() {
    return std::make_unique<PolkadotTrieCursorImpl>(trie_);
  }

  outcome::result<bool> TrieBatchBase::contains(const BufferView &key) const {
    return trie_->contains(key);
  }

  bool TrieBatchBase::empty() const {
    return trie_->empty();
  }

  outcome::result<std::optional<std::shared_ptr<TrieBatch>>>
  TrieBatchBase::createChildBatch(common::BufferView path) {
    OUTCOME_TRY(child_root_value, tryGet(path));
    auto child_root_hash =
        child_root_value ? common::Hash256::fromSpan(*child_root_value).value()
                         : serializer_->getEmptyRootHash();

    OUTCOME_TRY(unique_batch, createFromTrieHash(child_root_hash));
    auto batch = std::shared_ptr<TrieBatchBase>{std::move(unique_batch)};

    auto [it, success] = child_batches_.insert({path, batch});
    if (success) {
      return batch;
    }
    return std::nullopt;
  }

  outcome::result<void> TrieBatchBase::commitChildren(StateVersion version) {
    for (auto &[child_path, child_batch] : child_batches_) {
      OUTCOME_TRY(root, child_batch->commit(version));
      if (root == kEmptyRootHash) {
        OUTCOME_TRY(remove(child_path));
      } else {
        OUTCOME_TRY(put(child_path, common::BufferView{root}));
      }
    }
    child_batches_.clear();
    return outcome::success();
  }

}  // namespace kagome::storage::trie