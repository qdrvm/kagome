/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOCK_IMPL_HPP
#define KAGOME_CLOCK_IMPL_HPP

#include "transaction_pool/clock.hpp"

namespace kagome::transaction_pool {

  class SystemClock : public Clock<std::chrono::system_clock> {
   public:
    ~SystemClock() override = default;

    TimePoint now() const override {
      return std::chrono::system_clock::now();
    }
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_CLOCK_IMPL_HPP
