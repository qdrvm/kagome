/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE
#define KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE

#include "consensus/babe/types/epoch_digest.hpp"
#include "primitives/authority.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/common.hpp"
#include "scale/tie.hpp"

namespace kagome::primitives {
  struct DelayInChain {
    SCALE_TIE(1);
    uint32_t subchain_length = 0;

    DelayInChain() = default;
    explicit DelayInChain(uint32_t delay) : subchain_length(delay) {}
    virtual ~DelayInChain() = default;
  };

  struct AuthorityListChange {
    SCALE_TIE(2);
    AuthorityList authorities{};
    uint32_t subchain_length = 0;

    AuthorityListChange() = default;
    AuthorityListChange(AuthorityList authorities, uint32_t delay)
        : authorities(std::move(authorities)), subchain_length(delay) {}
    virtual ~AuthorityListChange() = default;
  };

  struct NextEpochData final : public consensus::babe::EpochDigest {
    using EpochDigest::EpochDigest;
  };

  struct NextConfigDataV1 final {
    SCALE_TIE(2);
    std::pair<uint64_t, uint64_t> ratio;
    AllowedSlots second_slot;
  };
  using NextConfigData = boost::variant<Unused<0>, NextConfigDataV1>;

  struct ScheduledChange final : public AuthorityListChange {
    using AuthorityListChange::AuthorityListChange;
  };

  struct ForcedChange final : public AuthorityListChange {
    ForcedChange() = default;

    ForcedChange(AuthorityList authorities,
                 uint32_t delay,
                 BlockNumber delay_start)
        : AuthorityListChange(authorities, delay), delay_start{delay_start} {}

    BlockNumber delay_start;

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 ForcedChange &change) {
      return s >> change.delay_start >> change.authorities
             >> change.subchain_length;
    }

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const ForcedChange &change) {
      return s << change.delay_start << change.authorities
               << change.subchain_length;
    }
  };

  struct OnDisabled {
    SCALE_TIE(1);
    uint32_t authority_index = 0;
  };

  struct Pause final : public DelayInChain {
    using DelayInChain::DelayInChain;
  };

  struct Resume final : public DelayInChain {
    using DelayInChain::DelayInChain;
  };

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_SCHEDULED_CHANGE
