/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/threshold_util.hpp"

#include <cmath>

#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/numeric.hpp>

namespace kagome::consensus {

  Threshold calculateThreshold(
      const std::pair<uint64_t, uint64_t> &c_pair,
      const std::vector<primitives::Authority> &authorities,
      primitives::AuthorityIndex authority_index) {
    double c = double(c_pair.first) / c_pair.second;

    using boost::adaptors::transformed;
    double theta =
        double(authorities[authority_index.index].babe_weight)
        / boost::accumulate(authorities | transformed([](auto &authority) {
                              return authority.babe_weight;
                            }),
                            double{0});

    using namespace boost::multiprecision;  // NOLINT
    cpp_rational p_rat(double{1} - pow(double{1} - c, theta));
    static const auto a = (uint256_t{1} << 128);
    return Threshold{a * numerator(p_rat) / denominator(p_rat)};
  }

}  // namespace kagome::consensus
