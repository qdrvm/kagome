/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_KADEMLIASTORAGEBACKEND
#define KAGOME_NETWORK_KADEMLIASTORAGEBACKEND

#include <libp2p/protocol/kademlia/storage_backend.hpp>

#include "common/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::network {

  class KademliaStorageBackend
      : public libp2p::protocol::kademlia::StorageBackend,
        public std::enable_shared_from_this<KademliaStorageBackend> {
   public:
    using ContentId = libp2p::protocol::kademlia::ContentId;
    using Value = libp2p::protocol::kademlia::Value;

    explicit KademliaStorageBackend(
        std::shared_ptr<kagome::storage::BufferStorage> storage);
    ~KademliaStorageBackend() override = default;

    outcome::result<void> putValue(ContentId key, Value value) override;

    outcome::result<Value> getValue(const ContentId &key) const override;

    outcome::result<void> erase(const ContentId &key) override;

   private:
    std::shared_ptr<storage::BufferStorage> storage_;
    common::Logger log_ = common::createLogger("KademliaStorage");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_KADEMLIASTORAGEBACKEND
