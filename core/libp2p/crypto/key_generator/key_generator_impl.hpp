/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator.hpp"

namespace libp2p::crypto {
  namespace random {
    class CSPRNG;
  }

  class KeyGeneratorImpl : public KeyGenerator {
   public:
    ~KeyGeneratorImpl() override = default;

    /**
     * @brief constructor, performs initialization
     * @param random_provider cryptographically secure random generator
     */
    explicit KeyGeneratorImpl(random::CSPRNG &random_provider);

    outcome::result<KeyPair> generateRsa(
        common::RSAKeyType key_type) const override;

    outcome::result<KeyPair> generateEd25519() const override;

    outcome::result<KeyPair> generateSecp256k1() const override;

    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const override;

    outcome::result<EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const override;

    std::vector<StretchedKey> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const kagome::common::Buffer &secret) const override;

    outcome::result<PrivateKey> importKey(const boost::filesystem::path &pem_path,
                                          std::string_view password) const override;

   private:
    /// \brief seeds openssl random generator
    void initialize();
    random::CSPRNG &random_provider_;  ///< random bytes generator
  };
}  // namespace libp2p::crypto
