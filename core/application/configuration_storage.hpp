/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIGURATION_STORAGE_HPP
#define KAGOME_CONFIGURATION_STORAGE_HPP

#include <libp2p/peer/peer_info.hpp>
#include "blockchain/genesis_raw_config.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/block.hpp"

namespace kagome::application {

  /**
   * Stores configuration of a kagome application and provides convenience
   * methods for accessing config parameters
   */
  class ConfigurationStorage {
   public:
    virtual ~ConfigurationStorage() = default;

    /**
     * @return genesis block of the chain
     */
    virtual blockchain::GenesisRawConfig getGenesis() const = 0;

    /**
     * Return ids of peer nodes of the current node
     */
    virtual std::vector<libp2p::peer::PeerInfo> getBootNodes() const = 0;

    /**
     * Return peers' session keys used in BABE
     */
    virtual std::vector<crypto::SR25519PublicKey> getSessionKeys() const = 0;

    /**
     * Return port to which extrinsic API RPC binds
     */
    virtual uint16_t getExtrinsicApiPort() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_HPP
