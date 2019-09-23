/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_SERVICE_HPP
#define KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_SERVICE_HPP

#include <memory>

#include "api/service/api_service.hpp"
#include "api/transport/impl/listener_impl.hpp"
#include "api/extrinsic/extrinsic_api.hpp"

namespace kagome::api {
  class ExtrinsicApiService : public ApiService {
   public:
    ExtrinsicApiService(std::shared_ptr<Listener> listener,
                        std::shared_ptr<ExtrinsicApi> api);

    ~ExtrinsicApiService() override = default;
  };
}  // namespace kagome::api
#endif  // KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_SERVICE_HPP
