/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/impl/system_clock.hpp"

namespace kagome::common {

  Clock::TimePoint SystemClock::now() const {
    return std::chrono::system_clock::now();
  }

}  // namespace kagome::common
