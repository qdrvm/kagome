/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/child_state/child_state_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/child_state/requests/get_keys.hpp"
#include "api/service/child_state/requests/get_keys_paged.hpp"
#include "api/service/child_state/requests/get_storage.hpp"
#include "api/service/child_state/requests/get_storage_hash.hpp"
#include "api/service/child_state/requests/get_storage_size.hpp"

namespace kagome::api::child_state {

  StateJrpcProcessor::StateJrpcProcessor(std::shared_ptr<JRpcServer> server,
                                         std::shared_ptr<ChildStateApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = kagome::api::Method<Request, ChildStateApi>;

  void StateJrpcProcessor::registerHandlers() {
    server_->registerHandler("childstate_getKeys",
                             Handler<request::GetKeys>(api_));

    server_->registerHandler("childstate_getKeysPaged",
                             Handler<request::GetKeysPaged>(api_));

    server_->registerHandler("childstate_getStorage",
                             Handler<request::GetStorage>(api_));

    // duplicate of `childstate_getStorage`. Required for compatibility with
    // some client
    server_->registerHandler("childstate_getStorageAt",
                             Handler<request::GetStorage>(api_));

    server_->registerHandler("childstate_getStorageHash",
                             Handler<request::GetStorageHash>(api_));

    // duplicate of `childstate_getStorageHash`. Required for compatibility with
    // some client
    server_->registerHandler("childstate_getStorageHashAt",
                             Handler<request::GetStorageHash>(api_));

    server_->registerHandler("childstate_getStorageSize",
                             Handler<request::GetStorageSize>(api_));

    // duplicate of `childstate_getStorageSize`. Required for compatibility with
    // some client
    server_->registerHandler("childstate_getStorageSizeAt",
                             Handler<request::GetStorageSize>(api_));
  }

}  // namespace kagome::api::child_state
