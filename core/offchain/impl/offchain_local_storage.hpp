/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGEIMPL
#define KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGEIMPL

#include "offchain/offchain_local_storage.hpp"

#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::offchain {

  class OffchainLocalStorageImpl final : public OffchainLocalStorage {
   public:
    explicit OffchainLocalStorageImpl(
        std::shared_ptr<storage::BufferStorage> storage);

    outcome::result<void> set(const common::Buffer &key,
                              common::Buffer value) override;

    outcome::result<void> clear(const common::Buffer &key) override;

    outcome::result<bool> compare_and_set(
        const common::Buffer &key,
        std::optional<std::reference_wrapper<const common::Buffer>> expected,
        common::Buffer value) override;

    outcome::result<common::Buffer> get(const common::Buffer &key) override;

   private:
    std::shared_ptr<storage::BufferStorage> storage_;

    std::mutex mutex_;
    log::Logger log_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINLOCALSTORAGEIMPL
