/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/chain_spec_impl.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <charconv>
#include <libp2p/multi/multiaddress.hpp>
#include <system_error>

#include "common/hexutil.hpp"
#include "common/visitor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::application, ChainSpecImpl::Error, e) {
  using E = kagome::application::ChainSpecImpl::Error;
  switch (e) {
    case E::MISSING_ENTRY:
      return "A required entry is missing in the config file";
    case E::MISSING_PEER_ID:
      return "Peer id is missing in a multiaddress provided in the config file";
    case E::PARSER_ERROR:
      return "Internal parser error";
    case E::NOT_IMPLEMENTED:
      return "Known entry name, but parsing not implemented";
  }
  return "Unknown error in ChainSpecImpl";
}

namespace kagome::application {

  namespace pt = boost::property_tree;

  outcome::result<std::shared_ptr<ChainSpecImpl>> ChainSpecImpl::loadFrom(
      const std::string &path) {
    // done so because of private constructor
    std::shared_ptr<ChainSpecImpl> config_storage{new ChainSpecImpl};
    config_storage->known_code_substitutes_ =
        std::make_shared<primitives::CodeSubstituteBlockIds>();
    OUTCOME_TRY(config_storage->loadFromJson(path));

    return config_storage;
  }

  outcome::result<void> ChainSpecImpl::loadFromJson(
      const std::string &file_path) {
    config_path_ = file_path;
    pt::ptree tree;
    try {
      pt::read_json(file_path, tree);
    } catch (pt::json_parser_error &e) {
      log_->error(
          "Parser error: {}, line {}: {}", e.filename(), e.line(), e.message());
      return Error::PARSER_ERROR;
    }

    OUTCOME_TRY(loadFields(tree));
    OUTCOME_TRY(loadGenesis(tree));
    OUTCOME_TRY(loadBootNodes(tree));

    return outcome::success();
  }

  outcome::result<void> ChainSpecImpl::loadFields(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(name, ensure("name", tree.get_child_optional("name")));
    name_ = name.get<std::string>("");

    OUTCOME_TRY(id, ensure("id", tree.get_child_optional("id")));
    id_ = id.get<std::string>("");

    // acquiring "chainType" value
    if (auto entry = tree.get_child_optional("chainType"); entry.has_value()) {
      chain_type_ = entry.value().get<std::string>("");
    } else {
      log_->warn(
          "Field 'chainType' was not specified in the chain spec. 'Live' by "
          "default.");
      chain_type_ = std::string("Live");
    }

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
      /*
       * Currently there is no special handling implemented for forkBlocks.
       * We read them, but do it in terms of legacy chain specs' support.
       * We assume we are safe even if there are some values specified.
       *
       * The kagome node currently is going to sync with the main fork only.
       * The full implementation of forkBlocks support is not worth it now but
       * can be added later on.
       */
      log_->warn(
          "A non-empty set of 'forkBlocks' encountered! They might not be "
          "taken into account!");
      for (auto &[_, fork_block] : fork_blocks_opt.value()) {
        OUTCOME_TRY(hash,
                    primitives::BlockHash::fromHexWithPrefix(
                        fork_block.get<std::string>("")));
        fork_blocks_.emplace(hash);
      }
    }

    auto bad_blocks_opt = tree.get_child_optional("badBlocks");
    if (bad_blocks_opt.has_value()
        && bad_blocks_opt.value().get<std::string>("") != "null") {
      /*
       * Currently there is no special handling implemented for badBlocks.
       * We read them, but do it in terms of legacy chain specs' support.
       * We assume we are safe even if there are some values specified.
       *
       * The kagome node currently is going to sync with the main fork only.
       * The full implementation of badBlocks support is not worth it now but
       * can be added later on.
       */
      log_->warn(
          "A non-empty set of 'badBlocks' encountered! They might not be "
          "taken into account!");
      for (auto &[_, bad_block] : bad_blocks_opt.value()) {
        OUTCOME_TRY(hash,
                    primitives::BlockHash::fromHexWithPrefix(
                        bad_block.get<std::string>("")));
        bad_blocks_.emplace(hash);
      }
    }

    auto consensus_engine_opt = tree.get_child_optional("consensusEngine");
    if (consensus_engine_opt.has_value()) {
      auto consensus_engine = consensus_engine_opt.value().get<std::string>("");
      if (consensus_engine != "null") {
        consensus_engine_.emplace(std::move(consensus_engine));
      }
    }

    auto code_substitutes_opt = tree.get_child_optional("codeSubstitutes");
    if (code_substitutes_opt.has_value()) {
      for (const auto &[block_id, code] : code_substitutes_opt.value()) {
        OUTCOME_TRY(block_id_parsed, parseBlockId(block_id));
        known_code_substitutes_->emplace(block_id_parsed);
      }
    }

    return outcome::success();
  }

  outcome::result<primitives::BlockId> ChainSpecImpl::parseBlockId(
      const std::string_view block_id_str) const {
    primitives::BlockId block_id;
    if (block_id_str.rfind("0x", 0) != std::string::npos) {  // Is it hash?
      OUTCOME_TRY(block_hash, common::Hash256::fromHexWithPrefix(block_id_str));
      block_id = block_hash;
    } else {
      primitives::BlockNumber block_num;
      auto res = std::from_chars(block_id_str.data(),
                                 block_id_str.data() + block_id_str.size(),
                                 block_num);
      if (res.ec != std::errc()) {
        return Error::PARSER_ERROR;
      }
      block_id = block_num;
    }
    return block_id;
  }

  outcome::result<common::Buffer> ChainSpecImpl::fetchCodeSubstituteByBlockInfo(
      const primitives::BlockInfo &block_info) const {
    if (known_code_substitutes_->count(block_info.hash) != 0
        && known_code_substitutes_->count(block_info.number) != 0) {
      return Error::MISSING_ENTRY;
    }

    pt::ptree tree;
    try {
      pt::read_json(config_path_, tree);
    } catch (pt::json_parser_error &e) {
      log_->error(
          "Parser error: {}, line {}: {}", e.filename(), e.line(), e.message());
      return Error::PARSER_ERROR;
    }

    auto code_substitutes_opt = tree.get_child_optional("codeSubstitutes");
    if (code_substitutes_opt.has_value()) {
      for (const auto &[_block_id, _code] : code_substitutes_opt.value()) {
        OUTCOME_TRY(block_id_parsed, parseBlockId(_block_id));
        if (visit_in_place(
                block_id_parsed,
                [&block_info](primitives::BlockNumber &arg) {
                  return arg == block_info.number;
                },
                [&block_info](primitives::BlockHash &arg) {
                  return arg == block_info.hash;
                })) {
          OUTCOME_TRY(code_processed, common::unhexWith0x(_code.data()));
          return outcome::success(code_processed);
        }
      }
    }
    return Error::MISSING_ENTRY;
  }

  outcome::result<void> ChainSpecImpl::loadGenesis(
      const boost::property_tree::ptree &tree) {
    OUTCOME_TRY(genesis_tree,
                ensure("genesis", tree.get_child_optional("genesis")));
    OUTCOME_TRY(genesis_raw_tree,
                ensure("genesis/raw", genesis_tree.get_child_optional("raw")));
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
    OUTCOME_TRY(boot_nodes,
                ensure("bootNodes", tree.get_child_optional("bootNodes")));
    for (auto &v : boot_nodes) {
      if (auto ma_res = libp2p::multi::Multiaddress::create(v.second.data())) {
        auto &&multiaddr = ma_res.value();
        if (auto peer_id_base58 = multiaddr.getPeerId();
            peer_id_base58.has_value()) {
          OUTCOME_TRY(libp2p::peer::PeerId::fromBase58(peer_id_base58.value()));
          boot_nodes_.emplace_back(std::move(multiaddr));
        } else {
          return Error::MISSING_PEER_ID;
        }
      } else {
        log_->warn("Unsupported multiaddress '{}'. Ignoring that boot node",
                   v.second.data());
      }
    }
    return outcome::success();
  }

}  // namespace kagome::application
