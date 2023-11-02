/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/rpc/rpc_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/rpc/requests/methods.hpp"

namespace kagome::api::rpc {

  RpcJRpcProcessor::RpcJRpcProcessor(std::shared_ptr<JRpcServer> server,
                                     std::shared_ptr<RpcApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = kagome::api::Method<Request, RpcApi>;

  void RpcJRpcProcessor::registerHandlers() {
    server_->registerHandler("rpc_methods", Handler<request::Methods>(api_));
  }

}  // namespace kagome::api::rpc
