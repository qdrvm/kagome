/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_TESTUTIL_LIBP2P_MA_GENERATOR_HPP
#define KAGOME_TEST_TESTUTIL_LIBP2P_MA_GENERATOR_HPP

#include <iostream>

#include "libp2p/multi/multiaddress.hpp"
#include "testutil/outcome.hpp"

namespace testutil {
  class MultiaddressGenerator {
    using Multiaddress = libp2p::multi::Multiaddress;

   public:
    inline MultiaddressGenerator(std::string prefix, uint16_t start_port)
        : prefix_{std::move(prefix)}, current_port_{start_port} {}

    inline Multiaddress nextMultiaddress() {
      std::string ma = prefix_ + std::to_string(current_port_++);
      EXPECT_OUTCOME_TRUE(res, Multiaddress::create(ma));
      return res;
    }

   private:
    std::string prefix_;
    uint16_t current_port_;
  };
}  // namespace testutil

#endif  // KAGOME_TEST_TESTUTIL_LIBP2P_MA_GENERATOR_HPP
