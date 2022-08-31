/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/internal/impl/internal_api_impl.hpp"

#include "log/logger.hpp"

namespace kagome::api {

  outcome::result<void> InternalApiImpl::setLogLevel(
      const std::string &group, const std::string &level_str) {
    OUTCOME_TRY(level, log::str2lvl(level_str));

    if (not log::setLevelOfGroup(group, level)) {
      return log::Error::WRONG_GROUP;
    }

    return outcome::success();
  }

}  // namespace kagome::api
