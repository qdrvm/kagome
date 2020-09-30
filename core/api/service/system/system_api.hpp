/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEMAPI
#define KAGOME_API_SYSTEMAPI

#include "application/configuration_storage.hpp"
#include "consensus/babe/babe.hpp"
#include "network/gossiper.hpp"

namespace kagome::api {

  /// Auxiliary class that providing access for some app's parts over RPC
  class SystemApi {
   public:
    virtual ~SystemApi() = default;

    virtual std::shared_ptr<application::ConfigurationStorage> getConfig()
        const = 0;

    virtual std::shared_ptr<consensus::Babe> getBabe() const = 0;

    virtual std::shared_ptr<network::Gossiper> getGossiper() const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SYSTEMAPI
