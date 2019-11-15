/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/pt_util.hpp"

#include "application/impl/config_reader/error.hpp"
#include "scale/scale.hpp"

namespace kagome::application {
  namespace pt = boost::property_tree;

  outcome::result<KagomeConfig> initConfigFromPropertyTree(
      const pt::ptree &tree) {
    KagomeConfig config;

    auto genesis_opt = tree.get_optional<std::string>("genesis");
    if (!genesis_opt) {
      return ConfigReaderError::MISSING_ENTRY;
    }
    OUTCOME_TRY(genesis_bytes, common::unhex(genesis_opt.value()));
    OUTCOME_TRY(genesis, scale::decode<primitives::Block>(genesis_bytes));
    config.genesis = std::move(genesis);

    for (auto &[_, authority] : tree.get_child("authorities")) {
      OUTCOME_TRY(bytes, common::unhex(authority.data()));
      crypto::ED25519PublicKey pkey;
      std::copy(bytes.begin(), bytes.end(), pkey.begin());
      config.authorities.push_back(pkey);
    }
    for (auto &[_, session_key] : tree.get_child("session_keys")) {
      OUTCOME_TRY(bytes, common::unhex(session_key.data()));
      crypto::SR25519PublicKey pkey;
      std::copy(bytes.begin(), bytes.end(), pkey.begin());
      config.session_keys.push_back(pkey);
    }
    for (auto &[_, peer_info] : tree.get_child("peer_ids")) {
      OUTCOME_TRY(id, libp2p::peer::PeerId::fromBase58(peer_info.data()));
      config.peer_ids.push_back(id);
    }

    return config;
  }

  outcome::result<void> updateConfigFromPropertyTree(
      KagomeConfig &config, const boost::property_tree::ptree &tree) {
    auto genesis_opt = tree.get_optional<std::string>("genesis");
    if (genesis_opt) {
      auto genesis_hex = genesis_opt.value();
      OUTCOME_TRY(genesis_bytes, common::unhex(genesis_hex));
      OUTCOME_TRY(genesis, scale::decode<primitives::Block>(genesis_bytes));
      config.genesis = std::move(genesis);
    }

    if (tree.get_child_optional("authorities").has_value()) {
      for (auto &[_, authority] : tree.get_child("authorities")) {
        OUTCOME_TRY(bytes, common::unhex(authority.data()));
        crypto::ED25519PublicKey pkey;
        std::copy(bytes.begin(), bytes.end(), pkey.begin());
        config.authorities.push_back(pkey);
      }
    }
    if (tree.get_child_optional("session_key").has_value()) {
      for (auto &[_, session_key] : tree.get_child("session_keys")) {
        OUTCOME_TRY(bytes, common::unhex(session_key.data()));
        crypto::SR25519PublicKey pkey;
        std::copy(bytes.begin(), bytes.end(), pkey.begin());
        config.session_keys.push_back(pkey);
      }
    }
    if (tree.get_child_optional("peer_ids").has_value()) {
      for (auto &[_, peer_info] : tree.get_child("peer_ids")) {
        OUTCOME_TRY(id, libp2p::peer::PeerId::fromBase58(peer_info.data()));
        config.peer_ids.push_back(id);
      }
    }

    return outcome::success();
  }

}  // namespace kagome::application
