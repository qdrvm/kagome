/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
#define KAGOME_CONFIGURATION_STORAGE_IMPL_HPP

#include "application/configuration_storage.hpp"

#include <boost/property_tree/ptree.hpp>

namespace kagome::application {

  class ConfigurationStorageImpl : public ConfigurationStorage {
   public:
    static outcome::result<std::shared_ptr<ConfigurationStorageImpl>> create(
        const std::string &config_path);

    ~ConfigurationStorageImpl() override = default;

    GenesisRawConfig getGenesis() const override;
    network::PeerList getBootNodes() const override;
    std::vector<crypto::SR25519PublicKey> getSessionKeys() const override;

   private:
    outcome::result<void> loadFromJson(const std::string &file_path);
    outcome::result<void> loadGenesis(const boost::property_tree::ptree &tree);
    outcome::result<void> loadBootNodes(
        const boost::property_tree::ptree &tree);
    outcome::result<void> loadSessionKeys(
        const boost::property_tree::ptree &tree);

    ConfigurationStorageImpl() = default;

    GenesisRawConfig genesis_;
    network::PeerList boot_nodes_;
    std::vector<crypto::SR25519PublicKey> session_keys_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
