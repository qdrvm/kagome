/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_REQUEST_SUBSCRIBE_STORAGE
#define KAGOME_API_REQUEST_SUBSCRIBE_STORAGE

#include <jsonrpc-lean/request.h>

#include <boost/optional.hpp>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class SubscribeStorage final {
   public:
    SubscribeStorage(const SubscribeStorage &) = delete;
    SubscribeStorage &operator=(const SubscribeStorage &) = delete;

    SubscribeStorage(SubscribeStorage &&) = default;
    SubscribeStorage &operator=(SubscribeStorage &&) = default;

    explicit SubscribeStorage(std::shared_ptr<StateApi> api)
        : api_(std::move(api)){};
    ~SubscribeStorage() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);
    outcome::result<uint32_t> execute();

   private:
    std::shared_ptr<StateApi> api_;
    std::vector<common::Buffer> key_buffers_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_API_REQUEST_SUBSCRIBE_STORAGE