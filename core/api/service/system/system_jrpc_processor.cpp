/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/system_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/system/requests/chain.hpp"
#include "api/service/system/requests/name.hpp"
#include "api/service/system/requests/properties.hpp"
#include "api/service/system/requests/version.hpp"

namespace kagome::api::system {

  SystemJrpcProcessor::SystemJrpcProcessor(std::shared_ptr<JRpcServer> server,
                                           std::shared_ptr<SystemApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = kagome::api::Method<Request, SystemApi>;

  void SystemJrpcProcessor::registerHandlers() {
    server_->registerHandler("system_name", Handler<request::Name>(api_));

    server_->registerHandler("system_version", Handler<request::Version>(api_));

    server_->registerHandler("system_chain", Handler<request::Chain>(api_));

    server_->registerHandler("system_properties",
                             Handler<request::Properties>(api_));
  }

}  // namespace kagome::api::system
