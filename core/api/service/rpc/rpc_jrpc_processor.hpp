/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_RPC_RPCJRPCPROCESSOR
#define KAGOME_API_RPC_RPCJRPCPROCESSOR

#include "api/jrpc/jrpc_processor.hpp"

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/rpc/rpc_api.hpp"

namespace kagome::api::rpc {

  class RpcJRpcProcessor : public JRpcProcessor {
   public:
    RpcJRpcProcessor(std::shared_ptr<JRpcServer> server,
                     std::shared_ptr<RpcApi> api);
    ~RpcJRpcProcessor() override = default;

    void registerHandlers() override;

   private:
    std::shared_ptr<RpcApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api::rpc
#endif  // KAGOME_API_RPC_RPCJRPCPROCESSOR
