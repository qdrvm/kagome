/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/random/impl/std_generator.hpp"

#include <random>

namespace libp2p::crypto::random {

  RandomGenerator::Buffer StdRandomGenerator::randomBytes(size_t len) {
    Buffer buffer(len, 0);
    for (auto i = 0; i < len; ++i) {
      unsigned char byte = distribution_(generator_);
      buffer[i] = byte;  // NOLINT
    }
    return buffer;
  }
}  // namespace libp2p::crypto::random
