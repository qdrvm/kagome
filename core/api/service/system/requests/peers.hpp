/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/base_request.hpp"

#include "account_next_index.hpp"
#include "api/service/system/system_api.hpp"
#include "chain.hpp"
#include "chain_type.hpp"
#include "health.hpp"

namespace kagome::api::system::request {

  /**
   * @brief Returns currently connected peers
   * @see
   *  https://github.com/w3f/PSPs/blob/master/PSPs/drafts/psp-6.md#1510-system_peers
   */
  struct Peers final : details::RequestType<jsonrpc::Value::Array> {
    explicit Peers(std::shared_ptr<SystemApi> &api) : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<Return> execute() override {
      auto &peer_manager = *api_->getPeerManager();

      jsonrpc::Value::Array result;
      result.reserve(peer_manager.activePeersNumber());

      peer_manager.enumeratePeerState(
          [&](const libp2p::PeerId &peer_id, const network::PeerState &state) {
            jsonrpc::Value::Struct peer;
            peer.emplace("PeerId", peer_id.toBase58());
            peer.emplace("roles",
                         state.roles.flags.authority ? "AUTHORITY"
                         : state.roles.flags.full    ? "FULL"
                         : state.roles.flags.light   ? "LIGHT"
                                                          : "NONE");
          peer.emplace("bestHash", makeValue(state.best_block.hash));
            peer.emplace("bestNumber", makeValue(state.best_block.number));

            result.emplace_back(std::move(peer));
            return true;
          });

      return result;
    }

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request
