/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "random_provider_std.hpp"

#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace libp2p::crypto::random {

  outcome::result<kagome::common::Buffer> RandomProviderStd::randomBytes(
      size_t number) const {
    boost::random_device generator;
    boost::random::uniform_int_distribution<uint8_t> distribution;
    Buffer bytes(number, 0);
    for (auto i = 0; i < number; ++i) {
      bytes[i] = distribution(generator);
    }

    return bytes;
  }
}  // namespace libp2p::crypto::random
