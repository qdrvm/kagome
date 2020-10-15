/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kademlia_value_storage.hpp"
#include "common/buffer.hpp"

namespace kagome::network {

  KademliaValueStorage::KademliaValueStorage(
      std::shared_ptr<kagome::storage::BufferStorage> storage)
      : _storage(std::move(storage)) {
    BOOST_ASSERT(_storage != nullptr);
  }

  outcome::result<void> KademliaValueStorage::putValue(
      KademliaValueStorage::ContentAddress key,
      KademliaValueStorage::Value value) {
    OUTCOME_TRY(_storage->put(common::Buffer(std::move(key.data)),
                              common::Buffer{value}));
    return outcome::success();
  }

  outcome::result<KademliaValueStorage::Value> KademliaValueStorage::getValue(
      const KademliaValueStorage::ContentAddress &key) const {
    OUTCOME_TRY(result, _storage->get(common::Buffer{key.data}));

    return std::move(result).toVector();
  }

  outcome::result<void> KademliaValueStorage::erase(
      const KademliaValueStorage::ContentAddress &key) {
    OUTCOME_TRY(_storage->remove(common::Buffer{key.data}));
    return outcome::success();
  }
}  // namespace kagome::network
