/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_server.hpp"
#include "api/service/rpc/rpc_api.hpp"

namespace kagome::api {

  class RpcApiImpl final : public RpcApi {
   public:
    RpcApiImpl(std::shared_ptr<JRpcServer> server);

    outcome::result<std::vector<std::string>> methods() const override;

   private:
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api
