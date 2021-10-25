/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain_persistent_storage.hpp"

#include "storage/database_error.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::offchain {

  namespace {
    common::Buffer internalKey(const common::Buffer &key) {
      const auto &prefix = storage::kOffchainWorkerStoragePrefix;
      return common::Buffer()
          .reserve(prefix.size() + key.size())
          .putBuffer(prefix)
          .putBuffer(key);
    }
  }  // namespace

  OffchainPersistentStorageImpl::OffchainPersistentStorageImpl(
      std::shared_ptr<storage::BufferStorage> storage)
      : storage_(std::move(storage)),
        log_(log::createLogger("OcwPersistentStorage", "offchain")) {
    BOOST_ASSERT(storage_);
  }

  outcome::result<void> OffchainPersistentStorageImpl::set(
      const common::Buffer &key, common::Buffer value) {
    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    return storage_->put(iKey, std::move(value));
  }

  outcome::result<void> OffchainPersistentStorageImpl::clear(
      const common::Buffer &key) {
    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    return storage_->remove(iKey);
  }

  outcome::result<bool> OffchainPersistentStorageImpl::compare_and_set(
      const common::Buffer &key,
      boost::optional<const common::Buffer &> expected,
      common::Buffer value) {
    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    auto get_res = storage_->get(iKey);
    if (get_res.has_error()) {
      if (get_res != outcome::failure(storage::DatabaseError::NOT_FOUND)) {
        return get_res.as_failure();
      }
    }

    boost::optional<const common::Buffer &> existing;
    if (get_res.has_value()) {
      existing = get_res.value();
    }

    if (existing == expected) {
      auto put_res = storage_->put(iKey, std::move(value));
      if (put_res.has_failure()) {
        return put_res.as_failure();
      }
      return true;
    }
    return false;
  }

  outcome::result<common::Buffer> OffchainPersistentStorageImpl::get(
      const common::Buffer &key) {
    auto iKey = internalKey(key);
    return storage_->get(iKey);
  }
}  // namespace kagome::offchain
