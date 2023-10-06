/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/base_request.hpp"

#include "log/logger.hpp"

namespace kagome::api::internal::request {

  struct SetLogLevel final
      : details::RequestType<void, std::string, std::optional<std::string>> {
    SetLogLevel(std::shared_ptr<InternalApi> &api) : api_(api){};

    outcome::result<Return> execute() override {
      const auto &group =
          getParam<1>().has_value() ? getParam<0>() : log::defaultGroupName;

      const auto &level =
          getParam<1>().has_value() ? getParam<1>().value() : getParam<0>();

      return api_->setLogLevel(group, level);
    }

   private:
    std::shared_ptr<InternalApi> api_;
  };

}  // namespace kagome::api::internal::request
