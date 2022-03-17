/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/kademlia_storage_backend.hpp"
#include "common/buffer.hpp"

namespace kagome::network {

  KademliaStorageBackend::KademliaStorageBackend(
      std::shared_ptr<kagome::storage::BufferStorage> storage)
      : storage_(std::move(storage)) {
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<void> KademliaStorageBackend::putValue(
      KademliaStorageBackend::ContentId key,
      KademliaStorageBackend::Value value) {
    return storage_->put(common::Buffer(std::move(key.data)),
                         common::Buffer{value});
  }

  outcome::result<KademliaStorageBackend::Value>
  KademliaStorageBackend::getValue(
      const KademliaStorageBackend::ContentId &key) const {
    OUTCOME_TRY(result, storage_->load(common::BufferView{key.data}));
    return std::vector<uint8_t>(result.begin(), result.end());
  }

  outcome::result<void> KademliaStorageBackend::erase(
      const KademliaStorageBackend::ContentId &key) {
    return storage_->remove(common::Buffer{key.data});
  }
}  // namespace kagome::network
