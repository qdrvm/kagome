/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_REQUEST_SUBSCRIBERUNTIMEVERSION
#define KAGOME_API_STATE_REQUEST_SUBSCRIBERUNTIMEVERSION

#include <jsonrpc-lean/request.h>
#include <boost/optional.hpp>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class SubscribeRuntimeVersion final {
   public:
    SubscribeRuntimeVersion(const SubscribeRuntimeVersion &) = delete;
    SubscribeRuntimeVersion &operator=(const SubscribeRuntimeVersion &) =
        delete;

    SubscribeRuntimeVersion(SubscribeRuntimeVersion &&) = default;
    SubscribeRuntimeVersion &operator=(SubscribeRuntimeVersion &&) = default;

    explicit SubscribeRuntimeVersion(std::shared_ptr<StateApi> api)
        : api_(std::move(api)){};
    ~SubscribeRuntimeVersion() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);
    outcome::result<void> execute();

   private:
    std::shared_ptr<StateApi> api_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_API_STATE_REQUEST_SUBSCRIBERUNTIMEVERSION
