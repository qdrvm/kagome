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
    enum class Error {
      NO_BATCH = 1,
    };

    explicit TrieStorageProviderImpl(
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<storage::trie::TrieSerializer> trie_serializer);

    ~TrieStorageProviderImpl() override = default;

    outcome::result<void> setToEphemeralAt(const common::Hash256 &state_root,
                                           OnDbRead on_db_read) override;

    outcome::result<void> setToPersistentAt(
        const common::Hash256 &state_root) override;

    std::shared_ptr<Batch> getCurrentBatch() const override;

    outcome::result<std::shared_ptr<PersistentBatch>> getChildBatchAt(
        const common::Buffer &root_path) override;

    outcome::result<storage::trie::RootHash> forceCommit(
        StateVersion version) override;

    outcome::result<void> startTransaction() override;
    outcome::result<void> rollbackTransaction() override;
    outcome::result<void> commitTransaction() override;

   private:
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<storage::trie::TrieSerializer> trie_serializer_;

    std::stack<std::shared_ptr<Batch>> stack_of_batches_;

    std::shared_ptr<Batch> current_batch_;

    // need to store it because it has to be the same in different runtime calls
    // to keep accumulated changes for commit to the main storage
    std::shared_ptr<PersistentBatch> persistent_batch_;

    std::unordered_map<common::Buffer, std::shared_ptr<PersistentBatch>>
        child_batches_;

    log::Logger logger_;
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, TrieStorageProviderImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
