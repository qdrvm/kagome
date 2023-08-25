/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>

#include "common/empty.hpp"
#include "common/tagged.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus::grandpa {

  using IsBlockFinalized = Tagged<bool, struct IsBlockFinalizedTag>;

  /**
   * @brief Node of scheduler tree. Contains actual authorities for the accorded
   * block and all its descendant blocks until any changes are applied. May
   * contain one of the changes.
   */
  class ScheduleNode : public std::enable_shared_from_this<ScheduleNode> {
   public:
    ScheduleNode() = default;

    ScheduleNode(const std::shared_ptr<const ScheduleNode> &ancestor,
                 primitives::BlockInfo block);

    /// Creates schedule node as root
    /// @result schedule node
    static std::shared_ptr<ScheduleNode> createAsRoot(
        std::shared_ptr<const primitives::AuthoritySet> current_authorities,
        primitives::BlockInfo block);

    void adjust(IsBlockFinalized finalized);

    /// Creates descendant schedule node for block
    /// @param block - target block
    /// @param finalized - true if block is finalized
    /// @result schedule node
    std::shared_ptr<ScheduleNode> makeDescendant(
        const primitives::BlockInfo &block, IsBlockFinalized finalized) const;

    using NoAction = Empty;

    struct ScheduledChange {
      SCALE_TIE(2);

      primitives::BlockNumber applied_block{};
      std::shared_ptr<const primitives::AuthoritySet> new_authorities{};
    };

    struct Pause {
      SCALE_TIE(1);

      primitives::BlockNumber applied_block{};
    };

    struct Resume {
      SCALE_TIE(1);

      primitives::BlockNumber applied_block{};
    };

    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const ScheduleNode &node) {
      return s << node.enabled << node.block << *node.authorities << node.action
               << node.forced_digests;
    }

    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, ScheduleNode &node) {
      return s >> node.enabled
          >> const_cast<primitives::BlockInfo &>(node.block) >> node.authorities
          >> node.action >> node.forced_digests;
    }

    const primitives::BlockInfo block{};
    std::weak_ptr<const ScheduleNode> parent;
    std::vector<std::shared_ptr<ScheduleNode>> descendants{};

    boost::variant<NoAction, ScheduledChange, Pause, Resume> action;
    std::vector<primitives::BlockInfo> forced_digests;
    std::shared_ptr<const primitives::AuthoritySet> authorities;
    bool enabled = true;
  };

}  // namespace kagome::consensus::grandpa
