/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_REQUEST_GET_STORAGE
#define KAGOME_API_REQUEST_GET_STORAGE

#include <jsonrpc-lean/request.h>

#include <optional>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class GetStorage final {
   public:
    GetStorage(GetStorage const &) = delete;
    GetStorage &operator=(GetStorage const &) = delete;

    GetStorage(GetStorage &&) = default;
    GetStorage &operator=(GetStorage &&) = default;

    explicit GetStorage(std::shared_ptr<StateApi> api) : api_(std::move(api)){};
    ~GetStorage() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::optional<common::BufferConstRef>> execute();

   private:
    std::shared_ptr<StateApi> api_;
    common::Buffer key_;
    std::optional<kagome::primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_STATE_JRPC_PROCESSOR_HPP
