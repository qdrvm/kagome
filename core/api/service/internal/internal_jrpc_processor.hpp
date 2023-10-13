/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/internal/internal_api.hpp"

namespace kagome::api::internal {
  /**
   * @brief extrinsic submission service implementation
   */
  class InternalJrpcProcessor : public JRpcProcessor {
   public:
    InternalJrpcProcessor(std::shared_ptr<JRpcServer> server,
                          std::shared_ptr<InternalApi> api);
    void registerHandlers() override;

   private:
    std::shared_ptr<InternalApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };
}  // namespace kagome::api::internal
