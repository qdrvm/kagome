/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "boost_generator.hpp"

namespace libp2p::crypto::random {

  void BoostRandomGenerator::randomBytes(unsigned char *out, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      unsigned char byte = distribution_(generator_);
      out[i] = byte;  // NOLINT
    }
  }
}  // namespace libp2p::crypto::random
