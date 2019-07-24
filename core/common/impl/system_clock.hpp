/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYSTEM_CLOCK_HPP
#define KAGOME_SYSTEM_CLOCK_HPP

#include "common/clock.hpp"

namespace kagome::common {

  class SystemClock : public Clock {
   public:
    ~SystemClock() override = default;

    TimePoint now() const override;
  };

}  // namespace kagome::common

#endif  // KAGOME_SYSTEM_CLOCK_HPP
