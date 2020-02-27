/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/state_api_service.hpp"

namespace kagome::api {

  StateApiService::StateApiService(
      std::shared_ptr<Listener> listener,
      std::shared_ptr<StateJrpcProcessor> processor)
      : ApiService(std::move(listener), std::move(processor)) {}

}  // namespace kagome::api
