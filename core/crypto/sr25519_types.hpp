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
#include "common/mp_utils.hpp"

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
    // an internal representation of the generated random value
    VRFPreOutput output{};
    // the proof to the output, serves as the verification of its randomness
    VRFProof proof{};

    bool operator==(const VRFOutput &other) const;
    bool operator!=(const VRFOutput &other) const;
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

namespace kagome::crypto {
  using Sr25519Seed = common::Blob<constants::sr25519::SEED_SIZE>;

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

  /**
   * @brief outputs object of type VRFOutput to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VRFOutput &o) {
    return s << o.output << o.proof;
  }

  /**
   * @brief decodes object of type VRFOutput from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, VRFOutput &o) {
    return s >> o.output >> o.proof;
  }

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_TYPES
