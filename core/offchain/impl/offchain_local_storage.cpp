/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/offchain_local_storage.hpp"

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

  OffchainLocalStorageImpl::OffchainLocalStorageImpl(
      std::shared_ptr<storage::SpacedStorage> storage)
      : storage_(storage->getSpace(storage::Space::kDefault)),
        log_(log::createLogger("OffchainLocalStorage", "offchain")) {
    BOOST_ASSERT(storage_);
  }

  outcome::result<void> OffchainLocalStorageImpl::set(
      const common::BufferView &key, common::Buffer value) {
    // TODO(xDimon):
    //  Need to implemented as soon as it will implemented in Substrate.
    //  Specification in not enough to implement it now.
    //  issue: https://github.com/soramitsu/kagome/issues/997
    throw std::invalid_argument("Off-chain local storage is unavailable yet");

    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    return storage_->put(iKey, std::move(value));
  }

  outcome::result<void> OffchainLocalStorageImpl::clear(
      const common::BufferView &key) {
    // TODO(xDimon):
    //  Need to implemented as soon as it will implemented in Substrate.
    //  Specification in not enough to implement it now.
    //  issue: https://github.com/soramitsu/kagome/issues/997
    throw std::invalid_argument("Off-chain local storage is unavailable yet");

    std::lock_guard lg(mutex_);
    return storage_->remove(key);
  }

  outcome::result<bool> OffchainLocalStorageImpl::compare_and_set(
      const common::BufferView &key,
      const std::optional<common::BufferView> &expected,
      common::Buffer value) {
    // TODO(xDimon):
    //  Need to implemented as soon as it will implemented in Substrate.
    //  Specification in not enough to implement it now.
    //  issue: https://github.com/soramitsu/kagome/issues/997
    throw std::invalid_argument("Off-chain local storage is unavailable yet");

    auto iKey = internalKey(key);
    std::lock_guard lg(mutex_);
    OUTCOME_TRY(get_opt, storage_->tryGet(iKey));

    std::optional<common::BufferView> existing;
    if (get_opt.has_value()) {
      existing = get_opt.value();
    }

    if (existing == expected) {
      OUTCOME_TRY(storage_->put(iKey, std::move(value)));
      return true;
    }
    return false;
  }

  outcome::result<common::Buffer> OffchainLocalStorageImpl::get(
      const common::BufferView &key) {
    // TODO(xDimon):
    //  Need to implemented as soon as it will implemented in Substrate.
    //  Specification in not enough to implement it now.
    //  issue: https://github.com/soramitsu/kagome/issues/997
    throw std::invalid_argument("Off-chain local storage is unavailable yet");

    auto iKey = internalKey(key);
    OUTCOME_TRY(value, storage_->get(iKey));
    return value.intoBuffer();
  }
}  // namespace kagome::offchain
