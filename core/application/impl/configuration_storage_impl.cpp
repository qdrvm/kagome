/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/configuration_storage_impl.hpp"
#include "common/hexutil.hpp"

namespace kagome::application {

  ConfigurationStorageImpl::ConfigurationStorageImpl(KagomeConfig config)
      : config_{std::move(config)} {}

  const primitives::Block &ConfigurationStorageImpl::getGenesis() const {
    return config_.genesis;
  }

  std::vector<libp2p::peer::PeerInfo> ConfigurationStorageImpl::getPeersInfo()
      const {
    return config_.peers_info;
  }

  std::vector<crypto::SR25519PublicKey>
  ConfigurationStorageImpl::getSessionKeys() const {
    return config_.session_keys;
  }

  std::vector<crypto::ED25519PublicKey>
  ConfigurationStorageImpl::getAuthorities() const {
    return config_.authorities;
  }

  uint32_t ConfigurationStorageImpl::getExtrinsicApiPort() const {
    return config_.api_ports.extrinsic_api_port;
  }

}  // namespace kagome::application
