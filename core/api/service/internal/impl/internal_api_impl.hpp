/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_INTERNALAPIIMPL
#define KAGOME_API_INTERNALAPIIMPL

#include "api/service/internal/internal_api.hpp"

namespace kagome::api {

  class InternalApiImpl : public InternalApi {
   public:
    outcome::result<void> setLogLevel(const std::string &group,
                                      const std::string &level) override;
  };
}  // namespace kagome::api

#endif  // KAGOME_API_INTERNALAPIIMPL
