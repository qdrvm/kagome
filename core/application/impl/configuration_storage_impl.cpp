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

    for (const auto &[key, value] : genesis_raw_tree) {
      // get rid of leading 0x for key and value and unhex
      OUTCOME_TRY(key_processed, common::unhexWith0x(key));
      OUTCOME_TRY(value_processed, common::unhexWith0x(value.data()));
      genesis_.emplace_back(key_processed, value_processed);
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
      boot_nodes_.peers.push_back(info);
    }
    return outcome::success();
  }

}  // namespace kagome::application
