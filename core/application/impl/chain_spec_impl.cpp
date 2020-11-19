/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/chain_spec_impl.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <libp2p/multi/multiaddress.hpp>

#include "application/impl/config_reader/error.hpp"
#include "application/impl/config_reader/pt_util.hpp"
#include "common/hexutil.hpp"

namespace kagome::application {

  outcome::result<std::shared_ptr<ChainSpecImpl>> ChainSpecImpl::create(const std::string &path) {
    auto config_storage = std::make_shared<ChainSpecImpl>(ChainSpecImpl());
    OUTCOME_TRY(config_storage->loadFromJson(path));

    return config_storage;
  }

  namespace pt = boost::property_tree;

  outcome::result<void> ChainSpecImpl::loadFromJson(
      const std::string &file_path) {
    pt::ptree tree;
    try {
      pt::read_json(file_path, tree);
    } catch (pt::json_parser_error &e) {
      spdlog::error(
          "Parser error: {}, line {}: {}", e.filename(), e.line(), e.message());
      return ConfigReaderError::PARSER_ERROR;
    }

    OUTCOME_TRY(loadFields(tree));
    OUTCOME_TRY(loadGenesis(tree));
    OUTCOME_TRY(loadBootNodes(tree));

    return outcome::success();
  }

  outcome::result<void> ChainSpecImpl::loadFields(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(name, ensure(tree.get_child_optional("name")));
    name_ = name.get<std::string>("");

    OUTCOME_TRY(id, ensure(tree.get_child_optional("id")));
    id_ = id.get<std::string>("");

    OUTCOME_TRY(chain_type, ensure(tree.get_child_optional("chainType")));
    chain_type_ = chain_type.get<std::string>("");

    auto telemetry_endpoints_opt =
        tree.get_child_optional("telemetryEndpoints");
    if (telemetry_endpoints_opt.has_value()
        && telemetry_endpoints_opt.value().get<std::string>("") != "null") {
      for (auto &[_, endpoint] : telemetry_endpoints_opt.value()) {
        if (auto it = endpoint.begin(); endpoint.size() >= 2) {
          auto &uri = it->second;
          auto &priority = (++it)->second;
          telemetry_endpoints_.emplace_back(uri.get<std::string>(""),
                                            priority.get<size_t>(""));
        }
      }
    }

    auto protocol_id_opt = tree.get_child_optional("protocolId");
    if (protocol_id_opt.has_value()) {
      auto protocol_id = protocol_id_opt.value().get<std::string>("");
      if (protocol_id != "null") {
        protocol_id_ = std::move(protocol_id);
      }
    }

    auto properties_opt = tree.get_child_optional("properties");
    if (properties_opt.has_value()
        && properties_opt.value().get<std::string>("") != "null") {
      for (auto &[propertyName, propertyValue] : properties_opt.value()) {
        properties_.emplace(propertyName, propertyValue.get<std::string>(""));
      }
    }

    auto fork_blocks_opt = tree.get_child_optional("forkBlocks");
    if (fork_blocks_opt.has_value()
        && fork_blocks_opt.value().get<std::string>("") != "null") {
      for (auto &[_, fork_block] : fork_blocks_opt.value()) {
        // TODO(xDimon): Ensure if implementation is correct, and remove return
        return ConfigReaderError::NOT_YET_IMPLEMENTED;  // NOLINT

        OUTCOME_TRY(hash,
                    primitives::BlockHash::fromHexWithPrefix(
                        fork_block.get<std::string>("")));
        fork_blocks_.emplace(hash);
      }
    }

    auto bad_blocks_opt = tree.get_child_optional("badBlocks");
    if (bad_blocks_opt.has_value()
        && bad_blocks_opt.value().get<std::string>("") != "null") {
      for (auto &[_, bad_block] : bad_blocks_opt.value()) {
        // TODO(xDimon): Ensure if implementation is correct, and remove return
        return ConfigReaderError::NOT_YET_IMPLEMENTED;  // NOLINT

        OUTCOME_TRY(hash,
                    primitives::BlockHash::fromHexWithPrefix(
                        bad_block.get<std::string>("")));
        fork_blocks_.emplace(hash);
      }
    }

    auto consensus_engine_opt = tree.get_child_optional("consensusEngine");
    if (consensus_engine_opt.has_value()) {
      auto consensus_engine = consensus_engine_opt.value().get<std::string>("");
      if (consensus_engine != "null") {
        consensus_engine_.emplace(std::move(consensus_engine));
      }
    }

    return outcome::success();
  }

  outcome::result<void> ChainSpecImpl::loadGenesis(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(genesis_tree, ensure(tree.get_child_optional("genesis")));
    OUTCOME_TRY(genesis_raw_tree,
                ensure(genesis_tree.get_child_optional("raw")));
    boost::property_tree::ptree top_tree;
    // v0.7+ format
    if (auto top_tree_opt = genesis_raw_tree.get_child_optional("top");
        top_tree_opt.has_value()) {
      top_tree = top_tree_opt.value();
    } else {
      // Try to fall back to v0.6
      top_tree = genesis_raw_tree.begin()->second;
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

  outcome::result<void> ChainSpecImpl::loadBootNodes(
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
