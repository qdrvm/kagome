/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/child_state/child_state_api.hpp"

namespace kagome::api::child_state {

  class ChildStateJrpcProcessor : public JRpcProcessor {
   public:
    ChildStateJrpcProcessor(std::shared_ptr<JRpcServer> server,
                            std::shared_ptr<ChildStateApi> api);
    ~ChildStateJrpcProcessor() override = default;

    void registerHandlers() override;

   private:
    std::shared_ptr<ChildStateApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api::child_state
