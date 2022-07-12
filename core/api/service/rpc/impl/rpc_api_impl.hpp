/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_RPCAPIIMPL
#define KAGOME_API_RPCAPIIMPL

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

#endif  // KAGOME_API_RPCAPIIMPL
