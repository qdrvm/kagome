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

    const std::string &name() const override {
      return name_;
    }

    const std::string &id() const override {
      return id_;
    }

    const std::string &chainType() const override {
      return chain_type_;
    }

    network::PeerList getBootNodes() const override {
      return boot_nodes_;
    }

    const std::vector<std::pair<std::string, size_t>> &telemetryEndpoints()
        const override {
      return telemetry_endpoints_;
    }

    const std::string &protocolId() const override {
      return protocol_id_;
    }

    boost::optional<std::reference_wrapper<const std::string>> getProperty(
        const std::string &property) const override {
      auto it = properties_.find(property);
      if (it != properties_.end()) {
        return {{it->second}};
      }
      return boost::none;
    }

    const std::set<primitives::BlockHash> &forkBlocks() const override {
      return fork_blocks_;
    }

    const std::set<primitives::BlockHash> &badBlocks() const override {
      return bad_blocks_;
    }

    boost::optional<std::string> consensusEngine() const override {
      return consensus_engine_;
    }

    GenesisRawConfig getGenesis() const override {
      return genesis_;
    }

   private:
    outcome::result<void> loadFromJson(const std::string &file_path);
    outcome::result<void> loadFields(const boost::property_tree::ptree &tree);
    outcome::result<void> loadGenesis(const boost::property_tree::ptree &tree);
    outcome::result<void> loadBootNodes(
        const boost::property_tree::ptree &tree);

    ConfigurationStorageImpl() = default;

    std::string name_;
    std::string id_;
    std::string chain_type_;
    network::PeerList boot_nodes_;
    std::vector<std::pair<std::string, size_t>> telemetry_endpoints_;
    std::string protocol_id_{"sup"};
    std::map<std::string, std::string> properties_;
    std::set<primitives::BlockHash> fork_blocks_;
    std::set<primitives::BlockHash> bad_blocks_;
    boost::optional<std::string> consensus_engine_;
    GenesisRawConfig genesis_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
