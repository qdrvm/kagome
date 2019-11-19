/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_CONFIG_HPP
#define KAGOME_KAGOME_CONFIG_HPP

#include <libp2p/peer/peer_info.hpp>
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/block.hpp"

namespace kagome::application {

  /**
   * Contains configuration parameters of a chain, such as genesis blocks or
   * authority keys
   */
  struct KagomeConfig {
    bool operator==(const KagomeConfig &rhs) const {
      return genesis == rhs.genesis
             and std::equal(peers_info.begin(),
                            peers_info.end(),
                            rhs.peers_info.begin(),
                            rhs.peers_info.end())
             and std::equal(session_keys.begin(),
                            session_keys.end(),
                            rhs.session_keys.begin(),
                            rhs.session_keys.end())
             and std::equal(authorities.begin(),
                            authorities.end(),
                            rhs.authorities.begin(),
                            rhs.authorities.end())
             and api_ports.extrinsic_api_port
                     == rhs.api_ports.extrinsic_api_port;
    }

    primitives::Block genesis;
    std::vector<libp2p::peer::PeerInfo> peers_info;
    std::vector<crypto::SR25519PublicKey> session_keys;
    std::vector<crypto::ED25519PublicKey> authorities;
    struct ApiPorts {
      uint32_t extrinsic_api_port;
    } api_ports;
  };

};  // namespace kagome::application

#endif  // KAGOME_KAGOME_CONFIG_HPP
