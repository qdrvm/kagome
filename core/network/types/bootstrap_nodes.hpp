/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include <boost/range/join.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"

namespace kagome::network {

  struct BootstrapNodes : public std::vector<libp2p::peer::PeerInfo> {
    BootstrapNodes() = delete;
    BootstrapNodes(const BootstrapNodes &) = delete;
    BootstrapNodes(BootstrapNodes &&) = delete;
    BootstrapNodes &operator=(const BootstrapNodes &) = delete;
    BootstrapNodes &operator=(BootstrapNodes &&) = delete;

    BootstrapNodes(const application::AppConfiguration &app_config,
                   const application::ChainSpec &chain_spec) {
      std::unordered_map<libp2p::peer::PeerId,
                         std::set<libp2p::multi::Multiaddress>>
          addresses_by_peer_id;

      for (auto &address :
           boost::join(chain_spec.bootNodes(), app_config.bootNodes())) {
        auto peer_id_base58_opt = address.getPeerId();
        if (peer_id_base58_opt) {
          auto peer_id_res =
              libp2p::peer::PeerId::fromBase58(peer_id_base58_opt.value());
          if (peer_id_res.has_value()) {
            addresses_by_peer_id[peer_id_res.value()].emplace(address);
          }
        }
      }

      reserve(addresses_by_peer_id.size());
      for (auto &item : addresses_by_peer_id) {
        emplace_back(libp2p::peer::PeerInfo{
            .id = item.first,
            .addresses = {std::make_move_iterator(item.second.begin()),
                          std::make_move_iterator(item.second.end())}});
      }
    }
  };

}  // namespace kagome::network
