/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/pt_util.hpp"

#include "application/impl/config_reader/error.hpp"
#include "scale/scale.hpp"

namespace kagome::application {

  outcome::result<KagomeConfig> initConfigFromPropertyTree(
      const boost::property_tree::ptree &tree) {
    auto genesis_opt = tree.get_optional<std::string>("genesis");
    if (!genesis_opt) {
      return ConfigReaderError::MISSING_ENTRY;
    }
    auto genesis_hex = genesis_opt.value();
    OUTCOME_TRY(genesis_bytes, common::unhex(genesis_hex));
    OUTCOME_TRY(genesis, scale::decode<primitives::Block>(genesis_bytes));

    KagomeConfig config;
    config.genesis = std::move(genesis);
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
    return outcome::success();
  }


}
