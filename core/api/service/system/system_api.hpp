/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEMAPI
#define KAGOME_API_SYSTEMAPI

#include "application/chain_spec.hpp"
#include "consensus/babe/babe.hpp"
#include "network/gossiper.hpp"
#include "primitives/account.hpp"

namespace kagome::api {

  /**
   * Auxiliary class that provides access to some app's parts for RPC methods
   */
  class SystemApi {
   public:
    virtual ~SystemApi() = default;

    virtual std::shared_ptr<application::ChainSpec> getConfig() const = 0;

    virtual std::shared_ptr<consensus::Babe> getBabe() const = 0;

    virtual std::shared_ptr<network::Gossiper> getGossiper() const = 0;

    virtual outcome::result<primitives::AccountNonce> getNonceFor(
        std::string_view account_address) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SYSTEMAPI
