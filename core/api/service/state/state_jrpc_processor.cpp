/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/state_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/state/requests/get_runtime_version.hpp"
#include "api/service/state/requests/get_storage.hpp"
#include "api/service/state/requests/subscribe_storage.hpp"
#include "api/service/state/requests/unsubscribe_storage.hpp"

namespace kagome::api::state {

  StateJrpcProcessor::StateJrpcProcessor(std::shared_ptr<JRpcServer> server,
                                         std::shared_ptr<StateApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = kagome::api::Method<Request, StateApi>;

  void StateJrpcProcessor::registerHandlers() {
    server_->registerHandler("state_getStorage",
                             Handler<request::GetStorage>(api_));

    server_->registerHandler("state_getRuntimeVersion",
                             Handler<request::GetRuntimeVersion>(api_));

    server_->registerHandler("state_subscribeStorage",
                             Handler<request::SubscribeStorage>(api_));

    server_->registerHandler("state_unsubscribeStorage",
                             Handler<request::UnsubscribeStorage>(api_));
  }

}  // namespace kagome::api::state
