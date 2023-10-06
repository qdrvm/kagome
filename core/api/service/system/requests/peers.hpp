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

      peer_manager.forEachPeer([&](auto &peer_id) {
        auto status_opt = peer_manager.getPeerState(peer_id);
        if (status_opt.has_value()) {
          auto &status = status_opt.value();

          jsonrpc::Value::Struct peer;
          peer.emplace("PeerId", peer_id.toBase58());
          peer.emplace("roles",
                       status.get().roles.flags.authority ? "AUTHORITY"
                       : status.get().roles.flags.full    ? "FULL"
                       : status.get().roles.flags.light   ? "LIGHT"
                                                          : "NONE");
          peer.emplace("bestHash", makeValue(status.get().best_block.hash));
          peer.emplace("bestNumber", makeValue(status.get().best_block.number));

          result.emplace_back(std::move(peer));
        }
      });

      return result;
    }

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request
