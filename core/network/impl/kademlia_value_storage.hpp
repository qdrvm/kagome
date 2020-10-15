/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_KADEMLIAVALUESTORAGE
#define KAGOME_NETWORK_KADEMLIAVALUESTORAGE

#include <libp2p/protocol/kad/value_store_backend.hpp>

#include "common/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::network {

  class KademliaValueStorage
      : public libp2p::protocol::kad::ValueStoreBackend,
        public std::enable_shared_from_this<KademliaValueStorage> {
   public:
    using ContentAddress = libp2p::protocol::kad::ContentAddress;
    using Value = libp2p::protocol::kad::Value;

    explicit KademliaValueStorage(
        std::shared_ptr<kagome::storage::BufferStorage> storage);
    ~KademliaValueStorage() override = default;


    outcome::result<void> putValue(ContentAddress key, Value value) override;

    outcome::result<Value> getValue(const ContentAddress &key) const override;

    outcome::result<void> erase(const ContentAddress &key) override;

   private:
    std::shared_ptr<storage::BufferStorage> _storage;
    common::Logger log_ = common::createLogger("KademliaStorage");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_KADEMLIAVALUESTORAGE
