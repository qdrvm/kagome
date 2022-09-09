/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE
#define KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

#include <boost/variant.hpp>

#include "common/empty.hpp"
#include "common/tagged.hpp"
#include "primitives/authority.hpp"

namespace kagome::authority {

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

    struct ForcedChange {
      primitives::BlockNumber delay_start{};
      size_t delay_length{};
      std::shared_ptr<const primitives::AuthoritySet> new_authorities{};

      friend inline ::scale::ScaleEncoderStream &operator<<(
          ::scale::ScaleEncoderStream &s, const ForcedChange &change) {
        return s << change.delay_start << change.delay_length
                 << *change.new_authorities;
      }

      friend inline ::scale::ScaleDecoderStream &operator>>(
          ::scale::ScaleDecoderStream &s, ForcedChange &change) {
        auto authority_list = std::make_shared<primitives::AuthoritySet>();
        s >> change.delay_start >> change.delay_length >> *authority_list;
        change.new_authorities = std::move(authority_list);
        return s;
      }
    };

    struct Pause {
      primitives::BlockNumber applied_block{};

      friend inline ::scale::ScaleEncoderStream &operator<<(
          ::scale::ScaleEncoderStream &s, const Pause &change) {
        return s << change.applied_block;
      }

      friend inline ::scale::ScaleDecoderStream &operator>>(
          ::scale::ScaleDecoderStream &s, Pause &change) {
        return s >> change.applied_block;
      }
    };

    struct Resume {
      primitives::BlockNumber applied_block{};

      friend inline ::scale::ScaleEncoderStream &operator<<(
          ::scale::ScaleEncoderStream &s, const Resume &change) {
        return s << change.applied_block;
      }

      friend inline ::scale::ScaleDecoderStream &operator>>(
          ::scale::ScaleDecoderStream &s, Resume &change) {
        return s >> change.applied_block;
      }
    };

    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const ScheduleNode &node) {
      return s << node.enabled << node.current_block
               << *node.current_authorities << node.action;
    }

    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, ScheduleNode &node) {
      s >> node.enabled
          >> const_cast<primitives::BlockInfo &>(node.current_block)
          >> node.current_authorities >> node.action;
      return s;
    }

    const primitives::BlockInfo current_block{};
    std::weak_ptr<const ScheduleNode> parent;
    std::vector<std::shared_ptr<ScheduleNode>> descendants{};
    boost::variant<NoAction, ScheduledChange, ForcedChange, Pause, Resume>
        action;
    std::shared_ptr<const primitives::AuthoritySet> current_authorities;
    bool enabled = true;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ScheduleNode &b) {
    s << b.block << b.actual_authorities << b.actual_authorities->id
      << b.enabled;
    if (b.scheduled_after != ScheduleNode::INACTIVE) {
      s << b.scheduled_after << b.scheduled_authorities
        << b.scheduled_authorities->id;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.forced_for != ScheduleNode::INACTIVE) {
      s << b.forced_for << b.forced_authorities << b.forced_authorities->id;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.pause_after != ScheduleNode::INACTIVE) {
      s << b.pause_after;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.resume_for != ScheduleNode::INACTIVE) {
      s << b.resume_for;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    s << b.descendants;
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ScheduleNode &b) {
    s >> const_cast<primitives::BlockInfo &>(b.block);  // NOLINT
    s >> b.actual_authorities;
    s >> const_cast<uint64_t &>(b.actual_authorities->id);  // NOLINT
    s >> b.enabled;
    primitives::BlockNumber bn;
    if (s >> bn, bn) {
      b.scheduled_after = bn;
      s >> b.scheduled_authorities;
      s >> const_cast<uint64_t &>(b.scheduled_authorities->id);  // NOLINT
    } else {
      b.scheduled_after = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.forced_for = bn;
      s >> b.forced_authorities;
      s >> const_cast<uint64_t &>(b.forced_authorities->id);  // NOLINT
    } else {
      b.forced_for = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.pause_after = bn;
    } else {
      b.pause_after = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.resume_for = bn;
    } else {
      b.resume_for = ScheduleNode::INACTIVE;
    }
    s >> b.descendants;
    return s;
  }

}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE
