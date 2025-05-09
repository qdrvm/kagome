/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iostream>

#include <qtils/test/outcome.hpp>

#include "libp2p/multi/multiaddress.hpp"

namespace testutil {
  class MultiaddressGenerator {
    using Multiaddress = libp2p::multi::Multiaddress;

   public:
    inline MultiaddressGenerator(std::string prefix, uint16_t start_port)
        : prefix_{std::move(prefix)}, current_port_{start_port} {}

    inline Multiaddress nextMultiaddress() {
      std::string ma = prefix_ + std::to_string(current_port_++);
      ASSERT_OUTCOME_SUCCESS(res, Multiaddress::create(ma));
      return res;
    }

   private:
    std::string prefix_;
    uint16_t current_port_;
  };
}  // namespace testutil
