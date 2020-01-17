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

  using VRFValue = boost::multiprecision::uint256_t;

  using VRFProof = std::array<uint8_t, constants::sr25519::vrf::PROOF_SIZE>;

  struct VRFOutput {
    VRFValue value;
    VRFProof proof{};

    bool operator==(const VRFOutput &other) const;
    bool operator!=(const VRFOutput &other) const;
  };

  using SR25519SecretKey = common::Blob<constants::sr25519::SECRET_SIZE>;

  using SR25519PublicKey = common::Blob<constants::sr25519::PUBLIC_SIZE>;

  struct SR25519Keypair {
    SR25519SecretKey secret_key;
    SR25519PublicKey public_key;

    SR25519Keypair() = default;

    bool operator==(const SR25519Keypair &other) const;
    bool operator!=(const SR25519Keypair &other) const;
  };

  using SR25519Signature =
      std::array<uint8_t, constants::sr25519::SIGNATURE_SIZE>;

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
    auto value_bytes = common::uint256_t_to_bytes(o.value);
    return s << value_bytes << o.proof;
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
    std::array<uint8_t, 32> value_bytes;
    s >> value_bytes >> o.proof;
    o.value = common::bytes_to_uint256_t(value_bytes);
    return s;
  }

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_TYPES
