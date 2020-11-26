/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEMAPIIMPL
#define KAGOME_API_SYSTEMAPIIMPL

#include "api/service/system/system_api.hpp"

namespace kagome::api {

  class SystemApiImpl final : public SystemApi {
   public:
    SystemApiImpl(std::shared_ptr<application::ChainSpec> config,
                  std::shared_ptr<consensus::Babe> babe,
                  std::shared_ptr<network::Gossiper> gossiper);

    std::shared_ptr<application::ChainSpec> getConfig()
        const override;

    std::shared_ptr<consensus::Babe> getBabe() const override;

    std::shared_ptr<network::Gossiper> getGossiper() const override;

   private:
    std::shared_ptr<application::ChainSpec> config_;
    std::shared_ptr<consensus::Babe> babe_;
    std::shared_ptr<network::Gossiper> gossiper_;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SYSTEMAPIIMPL
