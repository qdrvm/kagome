/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
#define KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL

#include "runtime/trie_storage_provider.hpp"

#include <stack>

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  class TrieStorageProviderImpl : public TrieStorageProvider {
   public:
    explicit TrieStorageProviderImpl(
        std::shared_ptr<storage::trie::TrieStorage> trie_storage);

    ~TrieStorageProviderImpl() override = default;

    outcome::result<void> setToEphemeral() override;
    outcome::result<void> setToEphemeralAt(
        const common::Hash256 &state_root) override;

    outcome::result<void> setToPersistent() override;
    outcome::result<void> setToPersistentAt(
        const common::Hash256 &state_root) override;

    std::shared_ptr<Batch> getCurrentBatch() const override;
    boost::optional<std::shared_ptr<PersistentBatch>> tryGetPersistentBatch()
        const override;
    bool isCurrentlyPersistent() const override;

    outcome::result<storage::trie::RootHash> forceCommit() override;

    outcome::result<void> startTransaction() override;
    outcome::result<void> rollbackTransaction() override;
    outcome::result<void> commitTransaction() override;

    storage::trie::RootHash getLatestRoot() const noexcept override;

   private:
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;

    std::stack<std::shared_ptr<Batch>> stack_of_batches_;

    std::shared_ptr<Batch> current_batch_;

    // need to store it because it has to be the same in different runtime calls
    // to keep accumulated changes for commit to the main storage
    std::shared_ptr<PersistentBatch> persistent_batch_;

    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
