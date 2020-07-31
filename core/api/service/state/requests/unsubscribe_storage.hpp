/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_REQUEST_UNSUBSCRIBE_STORAGE
#define KAGOME_API_REQUEST_UNSUBSCRIBE_STORAGE

#include <jsonrpc-lean/request.h>

#include <boost/optional.hpp>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class UnsubscribeStorage final {
   public:
    UnsubscribeStorage(const UnsubscribeStorage &) = delete;
    UnsubscribeStorage &operator=(const UnsubscribeStorage &) = delete;

    UnsubscribeStorage(UnsubscribeStorage &&) = default;
    UnsubscribeStorage &operator=(UnsubscribeStorage &&) = default;

    explicit UnsubscribeStorage(std::shared_ptr<StateApi> api)
        : api_(std::move(api)) { };
    ~UnsubscribeStorage() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);
    outcome::result<void> execute();

   private:
    std::shared_ptr<StateApi> api_;
    std::vector<uint32_t> subscriber_id_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_API_REQUEST_UNSUBSCRIBE_STORAGE