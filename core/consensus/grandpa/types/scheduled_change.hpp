/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/unused.hpp"
#include "consensus/grandpa/types/authority.hpp"
#include "primitives/common.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::grandpa {

  struct ScheduledChange {
    SCALE_TIE(2);
    AuthorityList authorities{};
    uint32_t subchain_length = 0;

    ScheduledChange() = default;
    ScheduledChange(AuthorityList authorities, uint32_t delay)
        : authorities(std::move(authorities)), subchain_length(delay) {}
  };

  struct ForcedChange {
    SCALE_TIE(3);
    primitives::BlockNumber delay_start = 0;
    AuthorityList authorities{};
    uint32_t subchain_length = 0;

    ForcedChange() = default;
    ForcedChange(AuthorityList authorities,
                 uint32_t delay,
                 primitives::BlockNumber delay_start)
        : delay_start(delay_start),
          authorities(std::move(authorities)),
          subchain_length(delay) {}
  };

  struct OnDisabled {
    SCALE_TIE(1);
    AuthorityIndex authority_index = 0;
  };

  struct Pause {
    SCALE_TIE(1);
    uint32_t subchain_length = 0;

    Pause() = default;
    explicit Pause(uint32_t delay) : subchain_length(delay) {}
  };

  struct Resume {
    SCALE_TIE(1);
    uint32_t subchain_length = 0;

    Resume() = default;
    explicit Resume(uint32_t delay) : subchain_length(delay) {}
  };

}  // namespace kagome::consensus::grandpa
