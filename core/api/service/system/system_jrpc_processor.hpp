/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/system/system_api.hpp"

namespace kagome::api::system {

  class SystemJrpcProcessor : public JRpcProcessor {
   public:
    SystemJrpcProcessor(std::shared_ptr<JRpcServer> server,
                        std::shared_ptr<SystemApi> api);
    ~SystemJrpcProcessor() override = default;

    void registerHandlers() override;

   private:
    std::shared_ptr<SystemApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api::system
