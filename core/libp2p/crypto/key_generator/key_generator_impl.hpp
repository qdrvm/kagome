/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator.hpp"

namespace libp2p::crypto {

  class KeyGeneratorImpl : public KeyGenerator {
   public:
    ~KeyGeneratorImpl() override = default;

    /**
     * @brief generates key pair of specified type
     * @param key_type key type option
     * @return new generated key pair
     */
    outcome::result<common::KeyPair> generateKeyPair(common::KeyType key_type) const override;

    /**
     * @brief derives public key from private key
     * @param private_key private key
     * @return derived public key
     */
    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const override;
    /**
     * Generate an ephemeral public key and return a function that will compute
     * the shared secret key
     * @param curve to be used in this ECDH
     * @return ephemeral key pair
     */
    outcome::result<common::EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const override;

    /**
     * Generate a set of keys for each party by stretching the shared key
     * @param cipher_type to be used
     * @param hash_type to be used
     * @param secret to be used
     * @return objects of type StretchedKey
     */
    std::vector<common::StretchedKey> keyStretcher(
        common::CipherType cipher_type, common::HashType hash_type,
        const kagome::common::Buffer &secret) const override;
  };
}  // namespace libp2p::crypto
