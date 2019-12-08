/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
#define KAGOME_CONFIGURATION_STORAGE_IMPL_HPP

#include "application/configuration_storage.hpp"

#include <boost/property_tree/ptree.hpp>
#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  class ConfigurationStorageImpl : public ConfigurationStorage {
   public:
    static outcome::result<std::shared_ptr<ConfigurationStorageImpl>> create(
        const std::string &config_path);

    ~ConfigurationStorageImpl() override = default;

    blockchain::GenesisRawConfig getGenesis() const override;
    std::vector<libp2p::peer::PeerInfo> getBootNodes() const override;
    std::vector<crypto::SR25519PublicKey> getSessionKeys() const override;

    uint16_t getExtrinsicApiPort() const override;

   private:
    outcome::result<void> loadFromJson(const std::string &file_path);
    outcome::result<void> loadGenesis(const boost::property_tree::ptree &tree);
    outcome::result<void> loadBootNodes(
        const boost::property_tree::ptree &tree);
    outcome::result<void> loadSessionKeys(
        const boost::property_tree::ptree &tree);

    ConfigurationStorageImpl() = default;
    KagomeConfig config_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
