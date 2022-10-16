/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babe_config_node.hpp"

namespace kagome::consensus {

  BabeConfigNode::BabeConfigNode(
      const std::shared_ptr<const BabeConfigNode> &ancestor,
      primitives::BlockInfo block)
      : current_block(block), parent(ancestor) {
    BOOST_ASSERT(ancestor != nullptr);
  }

  std::shared_ptr<BabeConfigNode> BabeConfigNode::createAsRoot(
      primitives::BlockInfo block,
      std::shared_ptr<const primitives::BabeConfiguration> config) {
    auto fake_parent = std::make_shared<BabeConfigNode>(BabeConfigNode());
    auto node = std::make_shared<BabeConfigNode>(fake_parent, block);
    node->current_config = std::move(config);
    return node;
  }

  std::shared_ptr<BabeConfigNode> BabeConfigNode::makeDescendant(
      const primitives::BlockInfo &target_block,
      std::optional<EpochNumber> target_epoch_number) const {
    auto node =
        std::make_shared<BabeConfigNode>(shared_from_this(), target_block);
    node->epoch_number = target_epoch_number.value_or(epoch_number);
    node->epoch_changed = node->epoch_number != epoch_number;
    if (not epoch_changed) {
      node->current_config = current_config;
      node->next_config = next_config;
    } else if (node->next_config.has_value()) {
      node->current_config = std::move(next_config).value();
      node->next_config.reset();
    }

    return node;
  }

}  // namespace kagome::consensus
