/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

extern "C" {
#include <bandersnatch_vrf/bandersnatch_vrf.h>
}
#include <boost/multiprecision/cpp_int.hpp>
#include <span>

#include "common/blob.hpp"
#include "common/int_serialization.hpp"
#include "primitives/math.hpp"
#include "scale/tie.hpp"

namespace kagome::crypto {
  namespace constants::bandersnatch {
    /**
     * Important constants to deal with bandersnatch
     */
    enum {
      // Max ring domain size.
      RING_DOMAIN_SIZE = 1024,

      // Short-Weierstrass form serialized sizes.
      PUBLIC_SIZE = 33,     // BANDERSNATCH_PUBLIC_SIZE,
      SIGNATURE_SIZE = 65,  // BANDERSNATCH_SIGNATURE_SIZE,
      RING_SIGNATURE_SERIALIZED_SIZE = 755,
      PREOUT_SERIALIZED_SIZE = 33,

      // Max size of serialized ring-vrf context params.
      //
      // This size is dependent on the ring domain size and the actual value
      // is equal to the SCALE encoded size of the `KZG` backend.
      RING_CONTEXT_SERIALIZED_SIZE = 147716,

      KEYPAIR_SIZE = 96,  // BANDERSNATCH_KEYPAIR_SIZE,
      SECRET_SIZE = 64,   // BANDERSNATCH_SECRET_SIZE,
      SEED_SIZE = 32,     // BANDERSNATCH_SEED_SIZE
    };

    namespace vrf {
      /**
       * Important constants to deal with vrf
       */
      enum {
        PROOF_SIZE = 64,   // BANDERSNATCH_VRF_PROOF_SIZE,
        OUTPUT_SIZE = 32,  // BANDERSNATCH_VRF_OUTPUT_SIZE
      };
    }  // namespace vrf

  }  // namespace constants::bandersnatch

  //  using VRFPreOutput =
  //      std::array<uint8_t, constants::bandersnatch::vrf::OUTPUT_SIZE>;
  //  using VRFThreshold = boost::multiprecision::uint128_t;
  //  using VRFProof = std::array<uint8_t,
  //  constants::bandersnatch::vrf::PROOF_SIZE>;
  //
  //  /**
  //   * Output of a verifiable random function.
  //   * Consists of pre-output, which is an internal representation of the
  //   * generated random value, and the proof to this value that servers as the
  //   * verification of its randomness.
  //   */
  //  struct VRFOutput {
  //    SCALE_TIE(2);
  //
  //    // an internal representation of the generated random value
  //    VRFPreOutput output{};
  //    // the proof to the output, serves as the verification of its randomness
  //    VRFProof proof{};
  //  };
  //
  //  /**
  //   * Output of a verifiable random function verification.
  //   */
  //  struct VRFVerifyOutput {
  //    // indicates if the proof is valid
  //    bool is_valid;
  //    // indicates if the value is less than the provided threshold
  //    bool is_less;
  //  };
}  // namespace kagome::crypto

KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           BandersnatchSecretKey,
                           constants::bandersnatch::SECRET_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           BandersnatchPublicKey,
                           constants::bandersnatch::PUBLIC_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           BandersnatchSignature,
                           constants::bandersnatch::SIGNATURE_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           BandersnatchSeed,
                           constants::bandersnatch::SEED_SIZE);

namespace kagome::crypto {
  //  template <typename D>
  //  struct Sr25519Signed {
  //    using Type = std::decay_t<D>;
  //    SCALE_TIE(2);
  //
  //    Type payload;
  //    Sr25519Signature signature;
  //  };

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
