/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/random/impl/std_generator.hpp"

#include <random>

namespace libp2p::crypto::random {

  void StdRandomGenerator::randomBytes(unsigned char *out, size_t len) {
    for (auto i = 0; i < len; ++i) {
      unsigned char byte = distribution_(generator_);
      out[i] = byte;  // NOLINT
    }
  }
}  // namespace libp2p::crypto::random
