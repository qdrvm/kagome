/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/offchain_storage_impl.hpp"
#include "storage/database_error.hpp"

namespace kagome::offchain {

  OffchainStorageImpl::OffchainStorageImpl(
      std::shared_ptr<storage::BufferStorage> storage)
      : storage_(std::move(storage)) {
    BOOST_ASSERT(storage_);
  }

  outcome::result<void> OffchainStorageImpl::set(const common::Buffer &key,
                                                 common::Buffer value) {
    std::lock_guard lg(mutex_);

    return storage_->put(key, std::move(value));
  }

  outcome::result<void> OffchainStorageImpl::clear(const common::Buffer &key) {
    std::lock_guard lg(mutex_);

    return storage_->remove(key);
  }

  outcome::result<bool> OffchainStorageImpl::compare_and_set(
      const common::Buffer &key,
      boost::optional<const common::Buffer &> expected,
      common::Buffer value) {
    std::lock_guard lg(mutex_);

    auto get_res = storage_->get(key);
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
      auto put_res = storage_->put(key, std::move(value));
      if (put_res.has_failure()) {
        return put_res.as_failure();
      }
      return true;
    }
    return false;
  }

  outcome::result<common::Buffer> OffchainStorageImpl::get(
      const common::Buffer &key) {
    return storage_->get(key);
  }
}  // namespace kagome::offchain
