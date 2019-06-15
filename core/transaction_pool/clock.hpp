/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOCK_HPP
#define KAGOME_CLOCK_HPP

#include <chrono>

namespace kagome::transaction_pool {


  class Clock {
   public:
      using Duration = std::chrono::milliseconds;
      using TimePoint = std::chrono::milliseconds;

    virtual ~Clock() = default;

    virtual TimePoint now() const = 0;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_CLOCK_HPP
