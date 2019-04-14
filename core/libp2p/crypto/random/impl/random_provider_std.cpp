/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "random_provider_std.hpp"

#include <random>

namespace libp2p::crypto::random {

  outcome::result<kagome::common::Buffer> RandomProviderStd::randomBytes(
      size_t number) const {
    std::random_device generator;
    std::uniform_int_distribution<uint8_t> distribution;
    Buffer bytes(number, 0);
    std::generate(bytes.begin(), bytes.end(), distribution(generator));
    return bytes;
  }
}  // namespace libp2p::crypto::random
