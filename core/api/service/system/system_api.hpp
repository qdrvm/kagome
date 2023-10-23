/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/chain_spec.hpp"
#include "consensus/timeline/timeline.hpp"
#include "network/peer_manager.hpp"
#include "primitives/account.hpp"

namespace kagome::api {

  /**
   * Auxiliary class that provides access to some app's parts for RPC methods
   */
  class SystemApi {
   public:
    virtual ~SystemApi() = default;

    virtual std::shared_ptr<application::ChainSpec> getConfig() const = 0;

    virtual std::shared_ptr<consensus::Timeline> getTimeline() const = 0;

    virtual std::shared_ptr<network::PeerManager> getPeerManager() const = 0;

    virtual outcome::result<primitives::AccountNonce> getNonceFor(
        std::string_view account_address) const = 0;
  };

}  // namespace kagome::api
