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
    return config_.genesis;
  }

  std::vector<libp2p::peer::PeerInfo> ConfigurationStorageImpl::getBootNodes()
      const {
    return config_.boot_nodes;
  }

  std::vector<crypto::SR25519PublicKey>
  ConfigurationStorageImpl::getSessionKeys() const {
    return config_.session_keys;
  }

  uint16_t ConfigurationStorageImpl::getExtrinsicApiPort() const {
    return config_.api_ports.extrinsic_api_port;
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
    OUTCOME_TRY(loadSessionKeys(tree));
    return outcome::success();
  }

  outcome::result<void> ConfigurationStorageImpl::loadGenesis(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(genesis_tree, ensure(tree.get_child_optional("genesis")));
    OUTCOME_TRY(genesis_raw_tree,
                ensure(genesis_tree.get_child_optional("raw")));

    for (const auto &[key, value] : genesis_raw_tree) {
      // get rid of leading 0x for key and value and unhex
      OUTCOME_TRY(key_processed, unhexWith0x(key));
      OUTCOME_TRY(value_processed, unhexWith0x(value.data()));
      config_.genesis.emplace_back(key_processed, value_processed);
    }
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
      libp2p::peer::PeerInfo info{.id = peer_id, .addresses = {multiaddr}};
      config_.boot_nodes.push_back(info);
    }
    return outcome::success();
  }

  outcome::result<void> ConfigurationStorageImpl::loadSessionKeys(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(session_keys, ensure(tree.get_child_optional("sessionKeys")));
    for (auto &v : session_keys) {
      std::string_view key_hex = v.second.data();
      OUTCOME_TRY(key, crypto::SR25519PublicKey::fromHex(key_hex.substr(2)));
      config_.session_keys.push_back(key);
    }
    return outcome::success();
  }

}  // namespace kagome::application
