/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_API_SERVICE_HPP
#define KAGOME_API_STATE_API_SERVICE_HPP

#include "api/service/api_service.hpp"

namespace kagome::api {

  class StateApiService : public ApiService {
   public:
    ~StateApiService() override = default;

    void start() override;
    void stop() override;
  };

}

#endif  // KAGOME_API_STATE_API_SERVICE_HPP
