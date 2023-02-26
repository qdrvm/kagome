/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_VRF_TYPES
#define KAGOME_CORE_CRYPTO_VRF_TYPES

extern "C" {
#include <schnorrkel/schnorrkel.h>
}
#include <boost/multiprecision/cpp_int.hpp>
#include <gsl/span>

#include "common/blob.hpp"
#include "primitives/math.hpp"
#include "common/int_serialization.hpp"
#include "scale/tie.hpp"

namespace kagome::crypto {
  namespace constants::sr25519 {
    /**
     * Important constants to deal with sr25519
     */
    enum {
      KEYPAIR_SIZE = SR25519_KEYPAIR_SIZE,
      SECRET_SIZE = SR25519_SECRET_SIZE,
      PUBLIC_SIZE = SR25519_PUBLIC_SIZE,
      SIGNATURE_SIZE = SR25519_SIGNATURE_SIZE,
      SEED_SIZE = SR25519_SEED_SIZE
    };

    namespace vrf {
      /**
       * Important constants to deal with vrf
       */
      enum {
        PROOF_SIZE = SR25519_VRF_PROOF_SIZE,
        OUTPUT_SIZE = SR25519_VRF_OUTPUT_SIZE
      };
    }  // namespace vrf

  }  // namespace constants::sr25519

  using VRFPreOutput =
      std::array<uint8_t, constants::sr25519::vrf::OUTPUT_SIZE>;
  using VRFThreshold = boost::multiprecision::uint128_t;
  using VRFProof = std::array<uint8_t, constants::sr25519::vrf::PROOF_SIZE>;

  /**
   * Output of a verifiable random function.
   * Consists of pre-output, which is an internal representation of the
   * generated random value, and the proof to this value that servers as the
   * verification of its randomness.
   */
  struct VRFOutput {
    SCALE_TIE(2);

    // an internal representation of the generated random value
    VRFPreOutput output{};
    // the proof to the output, serves as the verification of its randomness
    VRFProof proof{};
  };

  /**
   * Output of a verifiable random function verification.
   */
  struct VRFVerifyOutput {
    // indicates if the proof is valid
    bool is_valid;
    // indicates if the value is less than the provided threshold
    bool is_less;
  };
}  // namespace kagome::crypto

KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Sr25519SecretKey,
                           constants::sr25519::SECRET_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Sr25519PublicKey,
                           constants::sr25519::PUBLIC_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Sr25519Signature,
                           constants::sr25519::SIGNATURE_SIZE);
KAGOME_BLOB_STRICT_TYPEDEF(kagome::crypto,
                           Sr25519Seed,
                           constants::sr25519::SEED_SIZE);

namespace kagome::crypto {
  template <typename D>
  struct Sr25519Signed {
    using Type = std::decay_t<D>;
    SCALE_TIE(2);

    Type payload;
    Sr25519Signature signature;
  };

  struct Sr25519Keypair {
    Sr25519SecretKey secret_key;
    Sr25519PublicKey public_key;

    Sr25519Keypair() = default;

    bool operator==(const Sr25519Keypair &other) const;
    bool operator!=(const Sr25519Keypair &other) const;
  };

  struct Sr25519KeypairAndSeed : Sr25519Keypair {
    Sr25519Seed seed;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_TYPES
