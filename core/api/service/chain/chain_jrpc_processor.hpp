/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/chain/chain_api.hpp"

namespace kagome::api::chain {
  /**
   * @brief extrinsic submission service implementation
   */
  class ChainJrpcProcessor : public JRpcProcessor {
   public:
    ChainJrpcProcessor(std::shared_ptr<JRpcServer> server,
                       std::shared_ptr<ChainApi> api);
    void registerHandlers() override;

   private:
    std::shared_ptr<ChainApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };
}  // namespace kagome::api::chain
