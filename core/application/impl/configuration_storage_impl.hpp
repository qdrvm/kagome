/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
