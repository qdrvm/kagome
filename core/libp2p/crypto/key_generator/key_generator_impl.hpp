/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator.hpp"

namespace libp2p::crypto {
  namespace random {
    class CSPRNG;
  }

  /**
   * @class KeyGeneratorImpl implements KeyGenerator interface
   */
  class KeyGeneratorImpl : public KeyGenerator {
   public:
    ~KeyGeneratorImpl() override = default;

    /**
     * @brief seeds random generator
     * @param random_provider cryptographically secure random generator
     */
    void initialize(random::CSPRNG &random_provider);

    outcome::result<common::KeyPair> generateRsa(
        common::RSAKeyType key_type) const override;

    outcome::result<common::KeyPair> generateEd25519() const override;

    outcome::result<common::KeyPair> generateSecp256k1() const override;

    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const override;

    outcome::result<common::EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const override;

    std::vector<common::StretchedKey> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const kagome::common::Buffer &secret) const override;

    outcome::result<PrivateKey> import(
        boost::filesystem::path pem_path,
        std::string_view password) const override;

   private:
    /**
     * @return error if not initialized success otherwise
     */
    outcome::result<void> ensureInitialized() const;

    bool is_initialized_ = false;  ///< intitialization flag
  };
}  // namespace libp2p::crypto
