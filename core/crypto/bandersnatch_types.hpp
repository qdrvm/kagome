/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <span>

#include <bandersnatch_vrfs/bandersnatch_vrfs.h>

#include "common/blob.hpp"
#include "common/int_serialization.hpp"
#include "crypto/common.hpp"
#include "primitives/math.hpp"
#include "scale/tie.hpp"

namespace kagome::crypto::constants::bandersnatch {
  /// Important constants to deal with bandersnatch
  enum {  // NOLINT(performance-enum-size)
    SEED_SIZE = BANDERSNATCH_SEED_SIZE,
    SECRET_SIZE = BANDERSNATCH_SECRET_KEY_SIZE,
    PUBLIC_SIZE = BANDERSNATCH_PUBLIC_KEY_SIZE,
    KEYPAIR_SIZE = SECRET_SIZE + PUBLIC_SIZE,
    SIGNATURE_SIZE = BANDERSNATCH_SIGNATURE_SIZE,
    RING_SIGNATURE_SIZE = BANDERSNATCH_RING_SIGNATURE_SIZE,
  };

  namespace vrf {
    enum {  // NOLINT(performance-enum-size)
      OUTPUT_SIZE = BANDERSNATCH_PREOUT_SIZE,
    };
  }
}  // namespace kagome::crypto::constants::bandersnatch

KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           BandersnatchPublicKey,
                           constants::bandersnatch::PUBLIC_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           BandersnatchSignature,
                           constants::bandersnatch::SIGNATURE_SIZE);

namespace kagome::crypto {

  using BandersnatchSecretKey = PrivateKey<constants::bandersnatch::SECRET_SIZE,
                                           struct BandersnatchSecretKeyTag>;
  using BandersnatchSeed = PrivateKey<constants::bandersnatch::SEED_SIZE,
                                      struct BandersnatchSeedTag>;

  struct BandersnatchKeypair {
    BandersnatchSecretKey secret_key;
    BandersnatchPublicKey public_key;

    bool operator==(const BandersnatchKeypair &other) const = default;
    bool operator!=(const BandersnatchKeypair &other) const = default;
  };

  struct BandersnatchKeypairAndSeed : BandersnatchKeypair {
    BandersnatchSeed seed;
  };
}  // namespace kagome::crypto
