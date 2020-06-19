/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DUMMY_SYNC_PROTOCOL_CLIENT
#define KAGOME_DUMMY_SYNC_PROTOCOL_CLIENT

#include "network/sync_protocol_client.hpp"

#include "common/logger.hpp"

namespace kagome::network {

  class DummySyncProtocolClient
      : public network::SyncProtocolClient,
        public std::enable_shared_from_this<DummySyncProtocolClient> {
    using BlocksResponse = network::BlocksResponse;
    using BlocksRequest = network::BlocksRequest;

   public:
    DummySyncProtocolClient();

    void requestBlocks(
        const BlocksRequest &request,
        std::function<void(outcome::result<BlocksResponse>)> cb) override;

   private:
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_DUMMY_SYNC_PROTOCOL_CLIENT
