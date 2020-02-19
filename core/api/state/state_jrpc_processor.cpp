/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/state_jrpc_processor.hpp"

namespace kagome::api {

  void StateJrpcProcessor::registerHandlers() {
    registerHandler(
        "state_getStorage",
        [api_](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {

        });
  }

}  // namespace kagome::api
