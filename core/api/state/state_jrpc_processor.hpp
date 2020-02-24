/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_JRPC_PROCESSOR_HPP
#define KAGOME_STATE_JRPC_PROCESSOR_HPP

#include <mutex>

#include "api/jrpc/jrpc_processor.hpp"
#include "api/state/state_api.hpp"

namespace kagome::api {

  class StateJrpcProcessor : public JRPCProcessor {
   public:
    StateJrpcProcessor(std::shared_ptr<StateApi> api);
    ~StateJrpcProcessor() override = default;

    void registerHandlers() override;

   private:
    std::shared_ptr<StateApi> api_;
    std::mutex storage_mutex_;
  };

}  // namespace kagome::api
#endif  // KAGOME_STATE_JRPC_PROCESSOR_HPP
