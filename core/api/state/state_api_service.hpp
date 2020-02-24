/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_API_SERVICE_HPP
#define KAGOME_API_STATE_API_SERVICE_HPP

#include "api/service/api_service.hpp"
#include "api/state/state_jrpc_processor.hpp"

namespace kagome::api {

  class StateApiService : public ApiService {
   public:
    StateApiService(std::shared_ptr<Listener> listener,
                    std::shared_ptr<StateJrpcProcessor> processor);
    ~StateApiService() override = default;
  };

}

#endif  // KAGOME_API_STATE_API_SERVICE_HPP
