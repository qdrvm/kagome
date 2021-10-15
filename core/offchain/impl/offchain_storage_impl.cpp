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

  outcome::result<void> OffchainStorageImpl::set(common::Buffer key,
                                                 common::Buffer value) {
    std::lock_guard lg(mutex_);

    return storage_->put(key, std::move(value));
  }

  outcome::result<void> OffchainStorageImpl::clear(common::Buffer key) {
    std::lock_guard lg(mutex_);

    return storage_->remove(key);
  }

  outcome::result<bool> OffchainStorageImpl::compare_and_set(
      common::Buffer key, common::Buffer expected, common::Buffer value) {
    std::lock_guard lg(mutex_);

    auto get_res = storage_->get(key);
    if (get_res.has_value()) {
      auto &existed = get_res.value();
      if (existed == expected) {
        auto put_res = storage_->put(key, std::move(value));
        if (put_res.has_failure()) {
          return put_res.as_failure();
        }
        return true;
      }
      return false;
    }

    if (get_res == outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      return false;
    }

    return get_res.as_failure();
  }

  outcome::result<common::Buffer> OffchainStorageImpl::get(common::Buffer key) {
    return storage_->get(key);
  }
}  // namespace kagome::offchain
