/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/internal/internal_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/internal/requests/set_log_level.hpp"

namespace kagome::api::internal {

  InternalJrpcProcessor::InternalJrpcProcessor(
      std::shared_ptr<JRpcServer> server, std::shared_ptr<InternalApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = Method<Request, InternalApi>;

  void InternalJrpcProcessor::registerHandlers() {
    server_->registerHandler(
        "internal_setLogLevel", Handler<request::SetLogLevel>(api_), true);
  }

}  // namespace kagome::api::internal
