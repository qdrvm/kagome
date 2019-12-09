/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_CONFIG_HPP
#define KAGOME_KAGOME_CONFIG_HPP

#include <libp2p/peer/peer_info.hpp>
#include "blockchain/genesis_raw_config.hpp"
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
      return genesis == rhs.genesis and boot_nodes == rhs.boot_nodes
             and session_keys == rhs.session_keys
             and api_ports.extrinsic_api_port
                     == rhs.api_ports.extrinsic_api_port;
    }

    blockchain::GenesisRawConfig genesis;
    std::vector<libp2p::peer::PeerInfo> boot_nodes;
    std::vector<crypto::SR25519PublicKey> session_keys;
    struct ApiPorts {
      uint16_t extrinsic_api_port = 4224;
    } api_ports;
  };

};  // namespace kagome::application

#endif  // KAGOME_KAGOME_CONFIG_HPP
