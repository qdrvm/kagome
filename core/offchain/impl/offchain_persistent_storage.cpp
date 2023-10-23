/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain_persistent_storage.hpp"

#include "storage/database_error.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::offchain {

  namespace {
    common::Buffer internalKey(const common::BufferView &key) {
      const auto &prefix = storage::kOffchainWorkerStoragePrefix;
      return common::Buffer()
          .reserve(prefix.size() + key.size())
          .put(prefix)
          .put(key);
    }
  }  // namespace

  OffchainPersistentStorageImpl::OffchainPersistentStorageImpl(
      std::shared_ptr<storage::SpacedStorage> storage)
      : storage_(storage->getSpace(storage::Space::kDefault)),
        log_(log::createLogger("OffchainPersistentStorage", "offchain")) {
    BOOST_ASSERT(storage_);
  }

  outcome::result<void> OffchainPersistentStorageImpl::set(
      const common::BufferView &key, common::Buffer value) {
    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    return storage_->put(iKey, std::move(value));
  }

  outcome::result<void> OffchainPersistentStorageImpl::clear(
      const common::BufferView &key) {
    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    return storage_->remove(iKey);
  }

  outcome::result<bool> OffchainPersistentStorageImpl::compare_and_set(
      const common::BufferView &key,
      const std::optional<common::BufferView> &expected,
      common::Buffer value) {
    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    OUTCOME_TRY(get_opt, storage_->tryGet(iKey));

    std::optional<common::BufferView> existing;
    if (get_opt.has_value()) {
      existing = get_opt.value();
    }

    if ((not existing.has_value() and not expected.has_value())
        or (existing.has_value() and expected.has_value()
            and existing == expected)) {
      OUTCOME_TRY(storage_->put(iKey, std::move(value)));
      return true;
    }
    return false;
  }

  outcome::result<common::Buffer> OffchainPersistentStorageImpl::get(
      const common::BufferView &key) {
    auto iKey = internalKey(key);
    OUTCOME_TRY(value, storage_->get(iKey));
    return value.intoBuffer();
  }

}  // namespace kagome::offchain
