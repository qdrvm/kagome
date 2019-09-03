/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/extrinsic_api_service.hpp"

#include <chrono>
#include "api/extrinsic/extrinsic_jrpc_processor.hpp"
#include "api/transport/impl/listener_impl.hpp"

namespace kagome::api {
  using std::chrono_literals::operator""s;
  using std::chrono_literals::operator""ms;

  ExtrinsicApiService::ExtrinsicApiService(std::shared_ptr<Listener> listener,
                                           std::shared_ptr<ExtrinsicApi> api)
      : ApiService(std::move(listener),
                   std::make_shared<ExtrinsicJRPCProcessor>(std::move(api))) {}

}  // namespace kagome::api
