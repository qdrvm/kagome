/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/configuration_storage_impl.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <libp2p/multi/multiaddress.hpp>

#include "application/impl/config_reader/error.hpp"
#include "application/impl/config_reader/pt_util.hpp"
#include "common/hexutil.hpp"

namespace kagome::application {

  GenesisRawConfig ConfigurationStorageImpl::getGenesis() const {
    return genesis_;
  }

  network::PeerList ConfigurationStorageImpl::getBootNodes() const {
    return boot_nodes_;
  }

  outcome::result<std::shared_ptr<ConfigurationStorageImpl>>
  ConfigurationStorageImpl::create(const std::string &path) {
    auto config_storage =
        std::make_shared<ConfigurationStorageImpl>(ConfigurationStorageImpl());
    OUTCOME_TRY(config_storage->loadFromJson(path));

    return config_storage;
  }

  namespace pt = boost::property_tree;

  outcome::result<void> ConfigurationStorageImpl::loadFromJson(
      const std::string &file_path) {
    pt::ptree tree;
    try {
      pt::read_json(file_path, tree);
    } catch (pt::json_parser_error &e) {
      return ConfigReaderError::PARSER_ERROR;
    }

    OUTCOME_TRY(loadGenesis(tree));
    OUTCOME_TRY(loadBootNodes(tree));
    return outcome::success();
  }

  outcome::result<void> ConfigurationStorageImpl::loadGenesis(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(genesis_tree, ensure(tree.get_child_optional("genesis")));
    OUTCOME_TRY(genesis_raw_tree,
                ensure(genesis_tree.get_child_optional("raw")));
    boost::property_tree::ptree top_tree;
    // v0.7 format
    if(auto top_tree_opt = genesis_raw_tree.get_child_optional("top"); top_tree_opt.has_value()) {
      top_tree = std::move(top_tree_opt.value());
    } else {
      // Try to fall back to v0.6
      top_tree = std::move(genesis_raw_tree.begin()->second);
    }

    for (const auto &[key, value] : top_tree) {
      // get rid of leading 0x for key and value and unhex
      OUTCOME_TRY(key_processed, common::unhexWith0x(key));
      OUTCOME_TRY(value_processed, common::unhexWith0x(value.data()));
      genesis_.emplace_back(key_processed, value_processed);
    }
    // ignore child storages as they are not yet implemented

    return outcome::success();
  }

  outcome::result<void> ConfigurationStorageImpl::loadBootNodes(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(boot_nodes, ensure(tree.get_child_optional("bootNodes")));
    for (auto &v : boot_nodes) {
      OUTCOME_TRY(multiaddr,
                  libp2p::multi::Multiaddress::create(v.second.data()));
      OUTCOME_TRY(peer_id_base58, ensure(multiaddr.getPeerId()));

      OUTCOME_TRY(peer_id, libp2p::peer::PeerId::fromBase58(peer_id_base58));
      libp2p::peer::PeerInfo info{.id = std::move(peer_id),
                                  .addresses = {std::move(multiaddr)}};
      boot_nodes_.peers.push_back(info);
    }
    return outcome::success();
  }

}  // namespace kagome::application
