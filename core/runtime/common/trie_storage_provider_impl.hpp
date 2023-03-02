/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
#define KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL

#include "runtime/trie_storage_provider.hpp"

#include <stack>
#include <unordered_map>

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  class TrieStorageProviderImpl : public TrieStorageProvider {
   public:
    enum class Error { NO_BATCH = 1, UNFINISHED_TRANSACTIONS_LEFT };

    explicit TrieStorageProviderImpl(
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<storage::trie::TrieSerializer> trie_serializer);

    ~TrieStorageProviderImpl() override = default;

    outcome::result<void> setToEphemeralAt(
        const common::Hash256 &state_root) override;

    outcome::result<void> setToPersistentAt(
        const common::Hash256 &state_root) override;

    outcome::result<void> setTo(
        std::shared_ptr<storage::trie::TrieBatch> batch) override;

    std::shared_ptr<Batch> getCurrentBatch() const override;

    outcome::result<std::reference_wrapper<const storage::trie::TrieBatch>>
    getChildBatchAt(const common::Buffer &root_path) override;

    outcome::result<std::reference_wrapper<storage::trie::TrieBatch>>
    getMutableChildBatchAt(const common::Buffer &root_path) override;

    outcome::result<storage::trie::RootHash> commit(
        StateVersion version) override;

    outcome::result<void> startTransaction() override;
    outcome::result<void> rollbackTransaction() override;
    outcome::result<void> commitTransaction() override;

   private:
    outcome::result<std::optional<std::shared_ptr<storage::trie::TrieBatch>>>
    findChildBatchAt(const common::Buffer &root_path) const;

    outcome::result<std::shared_ptr<storage::trie::TrieBatch>>
    createBaseChildBatchAt(const common::Buffer &root_path);

    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<storage::trie::TrieSerializer> trie_serializer_;

    struct Transaction {
      // batch for the main trie in this transaction
      std::shared_ptr<storage::trie::TopperTrieBatch> main_batch;
      // batches for child tries in this transaction
      std::unordered_map<common::Buffer,
                         std::shared_ptr<storage::trie::TopperTrieBatch>>
          child_batches;
    };
    std::vector<Transaction> transaction_stack_;

    // base trie batch (i.e. not an overlay used for storage transactions)
    std::shared_ptr<Batch> base_batch_;

    // base child batches (i.e. not overlays used for storage transactions)
    std::unordered_map<common::Buffer, std::shared_ptr<Batch>> child_batches_;

    log::Logger logger_;
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, TrieStorageProviderImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
