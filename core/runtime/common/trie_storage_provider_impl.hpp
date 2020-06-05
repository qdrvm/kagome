/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
#define KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL

#include "runtime/trie_storage_provider.hpp"

#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  class TrieStorageProviderImpl : public TrieStorageProvider {
   public:
    explicit TrieStorageProviderImpl(
        std::shared_ptr<storage::trie::TrieStorage> trie_storage);

    virtual ~TrieStorageProviderImpl() = default;

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

   private:
    std::shared_ptr <storage::trie::TrieStorage> trie_storage_;

    std::shared_ptr<Batch> current_batch_;

    // need to store it because it has to be the same in different runtime calls
    // to keep accumulated changes for commit to the main storage
    std::shared_ptr<PersistentBatch> persistent_batch_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_TRIE_STORAGE_PROVIDER_IMPL
