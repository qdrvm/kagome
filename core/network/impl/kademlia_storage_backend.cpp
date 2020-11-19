/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kademlia_storage_backend.hpp"
#include "common/buffer.hpp"

namespace kagome::network {

  KademliaStorageBackend::KademliaStorageBackend(
      std::shared_ptr<kagome::storage::BufferStorage> storage)
      : _storage(std::move(storage)) {
    BOOST_ASSERT(_storage != nullptr);
  }

  outcome::result<void> KademliaStorageBackend::putValue(
		  KademliaStorageBackend::ContentId key,
		  KademliaStorageBackend::Value value) {
    OUTCOME_TRY(_storage->put(common::Buffer(std::move(key.data)),
                              common::Buffer{value}));
    return outcome::success();
  }

  outcome::result<KademliaStorageBackend::Value> KademliaStorageBackend::getValue(
      const KademliaStorageBackend::ContentId &key) const {
    OUTCOME_TRY(result, _storage->get(common::Buffer{key.data}));

    return std::move(result).toVector();
  }

  outcome::result<void> KademliaStorageBackend::erase(
      const KademliaStorageBackend::ContentId &key) {
    OUTCOME_TRY(_storage->remove(common::Buffer{key.data}));
    return outcome::success();
  }
}  // namespace kagome::network
