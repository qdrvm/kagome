/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/pt_util.hpp"

#include "application/impl/config_reader/error.hpp"
#include "scale/scale.hpp"

using libp2p::multi::Multiaddress;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;

namespace kagome::application {
  namespace pt = boost::property_tree;

  outcome::result<void> pt_unwrap(
      boost::optional<const pt::ptree &> tree_opt,
      bool update,
      const std::function<outcome::result<void>(const pt::ptree &)> &cb) {
    if (tree_opt.has_value()) {
      return cb(tree_opt.value());
    } else if (!update) {
      return ConfigReaderError::MISSING_ENTRY;
    }
    return outcome::success();
  }

  outcome::result<void> pt_foreach(
      const pt::ptree &node,
      const std::function<outcome::result<void>(const pt::ptree &)> &cb) {
    for (auto &[_, child] : node) {
      OUTCOME_TRY(cb(child));
    }
    return outcome::success();
  }

  outcome::result<KagomeConfig> processConfigFromPropertyTree(
      const pt::ptree &tree, boost::optional<KagomeConfig &> conf) {
    bool update = conf.has_value();
    KagomeConfig config;
    if (update) config = conf.value();

    auto genesis_opt = tree.get_optional<std::string>("genesis");
    if (genesis_opt) {
      OUTCOME_TRY(genesis_bytes, common::unhex(genesis_opt.value()));
      OUTCOME_TRY(genesis, scale::decode<primitives::Block>(genesis_bytes));
      config.genesis = std::move(genesis);
    } else if (!update) {
      return ConfigReaderError::MISSING_ENTRY;
    }

    auto extrinsic_api_port_opt =
        tree.get_child("api_ports").get_optional<uint16_t>("extrinsic");
    if (extrinsic_api_port_opt) {
      config.api_ports.extrinsic_api_port = extrinsic_api_port_opt.value();
    } else if (!update) {
      return ConfigReaderError::MISSING_ENTRY;
    }

    OUTCOME_TRY(pt_unwrap(
        tree.get_child_optional("authorities"),
        update,
        [&config](const pt::ptree &node) -> outcome::result<void> {
          return pt_foreach(
              node,
              [&config](const pt::ptree &authority) -> outcome::result<void> {
                OUTCOME_TRY(bytes, common::unhex(authority.data()));
                crypto::ED25519PublicKey pkey;
                std::copy(bytes.begin(), bytes.end(), pkey.begin());
                config.authorities.push_back(pkey);
                return outcome::success();
              });
        }));

    OUTCOME_TRY(pt_unwrap(
        tree.get_child_optional("session_keys"),
        update,
        [&config](const pt::ptree &node) -> outcome::result<void> {
          return pt_foreach(
              node,
              [&config](const pt::ptree &session_key) -> outcome::result<void> {
                OUTCOME_TRY(bytes, common::unhex(session_key.data()));
                crypto::SR25519PublicKey pkey;
                std::copy(bytes.begin(), bytes.end(), pkey.begin());
                config.session_keys.push_back(pkey);
                return outcome::success();
              });
        }));

    OUTCOME_TRY(pt_unwrap(
        tree.get_child_optional("peers_info"),
        update,
        [&config](const pt::ptree &node) -> outcome::result<void> {
          return pt_foreach(
              node,
              [&config](
                  const pt::ptree &peer_info_data) -> outcome::result<void> {
                auto peer_id = peer_info_data.get_child("id");
                OUTCOME_TRY(id, PeerId::fromBase58(peer_id.data()));
                PeerInfo peer_info{id, {}};
                for (auto &[_, address_data] :
                     peer_info_data.get_child("addresses")) {
                  OUTCOME_TRY(address,
                              Multiaddress::create(address_data.data()));
                  peer_info.addresses.push_back(address);
                }
                config.peers_info.push_back(peer_info);
                return outcome::success();
              });
        }));
    if(update) {
      conf.value() = config;
    }
    return config;
  }

  outcome::result<KagomeConfig> initConfigFromPropertyTree(
      const pt::ptree &tree) {
    return processConfigFromPropertyTree(tree, boost::none);
  }

  outcome::result<void> updateConfigFromPropertyTree(
      KagomeConfig &config, const boost::property_tree::ptree &tree) {
    auto res = processConfigFromPropertyTree(tree, config);
    if (res) {
      return outcome::success();
    } else {
      return res.error();
    }
  }

}  // namespace kagome::application
