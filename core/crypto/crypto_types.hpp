/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_VRF_TYPES
#define KAGOME_CORE_CRYPTO_VRF_TYPES

extern "C" {
#include <sr25519/sr25519.h>
}
#include <boost/multiprecision/cpp_int.hpp>
#include <gsl/span>
#include "common/blob.hpp"

namespace kagome::crypto {

  using VRFValue = boost::multiprecision::uint256_t;

  using VRFProof = std::array<uint8_t, SR25519_VRF_PROOF_SIZE>;

  struct VRFOutput {
    VRFValue value;
    VRFProof proof{};

    bool operator==(const VRFOutput &other) const;
    bool operator!=(const VRFOutput &other) const;
  };

  using SR25519SecretKey = common::Blob<SR25519_SECRET_SIZE>;

  using SR25519PublicKey = common::Blob<SR25519_PUBLIC_SIZE>;

  struct SR25519Keypair {
    SR25519SecretKey secret_key;
    SR25519PublicKey public_key;

    SR25519Keypair() = default;

    explicit SR25519Keypair(gsl::span<uint8_t, SR25519_KEYPAIR_SIZE> kp);
  };

  using SR25519Signature = std::array<uint8_t, SR25519_SIGNATURE_SIZE>;

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
    return s << o.value << o.proof;
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
    return s >> o.value >> o.proof;
  }

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_TYPES
