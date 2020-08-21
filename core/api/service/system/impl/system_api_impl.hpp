/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEMAPIIMPL
#define KAGOME_API_SYSTEMAPIIMPL

#include "api/service/system/system_api.hpp"

#include "application/configuration_storage.hpp"

namespace kagome::api {

  class SystemApiImpl final : public SystemApi {
   public:
    SystemApiImpl(std::shared_ptr<application::ConfigurationStorage> config);

    std::shared_ptr<application::ConfigurationStorage> getConfig()
        const override;

   private:
    std::shared_ptr<application::ConfigurationStorage> config_;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SYSTEMAPIIMPL
