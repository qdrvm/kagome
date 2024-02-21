/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"
#include "injector/lazy.hpp"

namespace kagome::network {
  class Beefy;
}  // namespace kagome::network

namespace kagome::blockchain {
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::api {
  class BeefyRpc : public JRpcProcessor,
                   public std::enable_shared_from_this<BeefyRpc> {
   public:
    BeefyRpc(
        std::shared_ptr<JRpcServer> server,
        LazySPtr<network::Beefy> beefy,
        LazySPtr<blockchain::BlockHeaderRepository> block_header_repository);

    void registerHandlers() override;

   private:
    std::shared_ptr<JRpcServer> server_;
    LazySPtr<network::Beefy> beefy_;
    LazySPtr<blockchain::BlockHeaderRepository> block_header_repository_;
  };
}  // namespace kagome::api
