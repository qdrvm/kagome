/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/state_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/state/requests/call.hpp"
#include "api/service/state/requests/get_keys_paged.hpp"
#include "api/service/state/requests/get_metadata.hpp"
#include "api/service/state/requests/get_read_proof.hpp"
#include "api/service/state/requests/get_runtime_version.hpp"
#include "api/service/state/requests/get_storage.hpp"
#include "api/service/state/requests/query_storage.hpp"
#include "api/service/state/requests/subscribe_runtime_version.hpp"
#include "api/service/state/requests/subscribe_storage.hpp"
#include "api/service/state/requests/unsubscribe_runtime_version.hpp"
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
    server_->registerHandler("state_call", Handler<request::Call>(api_));

    server_->registerHandler("state_getKeysPaged",
                             Handler<request::GetKeysPaged>(api_));

    server_->registerHandler("state_getStorage",
                             Handler<request::GetStorage>(api_));

    // duplicate of `state_getStorage`. Required for compatibility with
    // some client
    server_->registerHandler("state_getStorageAt",
                             Handler<request::GetStorage>(api_));

    server_->registerHandler(
        "state_queryStorage", Handler<request::QueryStorage>(api_), true);
    server_->registerHandler("state_queryStorageAt",
                             Handler<request::QueryStorageAt>(api_));

    server_->registerHandler("state_getReadProof",
                             Handler<request::GetReadProof>(api_));

    server_->registerHandler("state_getRuntimeVersion",
                             Handler<request::GetRuntimeVersion>(api_));

    // duplicate of `state_getRuntimeVersion`. Required for compatibility with
    // some client libraries
    server_->registerHandler("chain_getRuntimeVersion",
                             Handler<request::GetRuntimeVersion>(api_));

    server_->registerHandler("state_subscribeRuntimeVersion",
                             Handler<request::SubscribeRuntimeVersion>(api_));

    server_->registerHandler("state_subscribeStorage",
                             Handler<request::SubscribeStorage>(api_));

    server_->registerHandler("state_unsubscribeStorage",
                             Handler<request::UnsubscribeStorage>(api_));

    server_->registerHandler("state_unsubscribeRuntimeVersion",
                             Handler<request::UnsubscribeRuntimeVersion>(api_));

    server_->registerHandler("state_getMetadata",
                             Handler<request::GetMetadata>(api_));
  }

}  // namespace kagome::api::state
