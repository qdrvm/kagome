/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABECONFIGNODE
#define KAGOME_CONSENSUS_BABECONFIGNODE

#include <memory>

#include "primitives/block.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::babe {

  using IsBlockFinalized = Tagged<bool, struct IsBlockFinalizedTag>;

  class BabeConfigNode final
      : public std::enable_shared_from_this<BabeConfigNode> {
   public:
    BabeConfigNode() = default;

    BabeConfigNode(const std::shared_ptr<const BabeConfigNode> &ancestor,
                   primitives::BlockInfo block);

    /// Creates node as root
    /// @param block - target block
    /// @param config - config associated with provided block
    /// @result node
    static std::shared_ptr<BabeConfigNode> createAsRoot(
        primitives::BlockInfo block,
        std::shared_ptr<const primitives::BabeConfiguration> config);

    /// Creates descendant schedule node for block
    /// @param block - target block
    /// @param epoch_number - optional number to inform if provided block of
    /// other epoch
    /// @result descendant node
    std::shared_ptr<BabeConfigNode> makeDescendant(
        const primitives::BlockInfo &block,
        std::optional<EpochNumber> epoch_number = std::nullopt) const;

    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const BabeConfigNode &node) {
      return s << node.block << node.epoch << node.config << node.next_config;
    }

    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, BabeConfigNode &node) {
      return s >> const_cast<primitives::BlockInfo &>(node.block) >> node.epoch
             >> node.config >> node.next_config;
    }

    const primitives::BlockInfo block{};
    std::weak_ptr<const BabeConfigNode> parent;
    std::vector<std::shared_ptr<BabeConfigNode>> descendants{};

    EpochNumber epoch{};
    bool epoch_changed = false;
    std::shared_ptr<const primitives::BabeConfiguration> config;
    std::optional<std::shared_ptr<const primitives::BabeConfiguration>>
        next_config;
  };

};  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABECONFIGNODE
