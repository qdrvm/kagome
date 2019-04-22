/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_IMPL_CRYPTO_PROVIDER_IMPL_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_IMPL_CRYPTO_PROVIDER_IMPL_HPP

#include "libp2p/crypto/aes/aes_provider.hpp"
#include "libp2p/crypto/crypto_provider.hpp"
#include "libp2p/crypto/hmac/hmac_provider.hpp"
#include "libp2p/crypto/marshaller/key_marshaller.hpp"

namespace libp2p::crypto {
  class CryptoProviderImpl : public CryptoProvider {
   public:
    ~CryptoProviderImpl() override = default;

    outcome::result<Buffer> aesEncrypt(const common::Aes128Secret &secret,
                                       const Buffer &data) const override;

    outcome::result<Buffer> aesEncrypt(const common::Aes256Secret &secret,
                                       const Buffer &data) const override;

    outcome::result<Buffer> aesDecrypt(const common::Aes128Secret &secret,
                                       const Buffer &data) const override;

    outcome::result<Buffer> aesDecrypt(const common::Aes256Secret &secret,
                                       const Buffer &data) const override;

    outcome::result<Buffer> hmacDigest(common::HashType hash,
                                       const Buffer &secret,
                                       const Buffer &data) override;

    common::KeyPair generateEd25519Keypair() const override;

    common::KeyPair generateRSAKeypair(
        common::RSAKeyType key_type) const override;

    common::EphemeralKeyPair generateEphemeralKeyPair(
        common::CurveType curve) const override;

    std::vector<common::StretchedKey> keyStretcher(
        common::CipherType cipher_type, common::HashType hash_type,
        const Buffer &secret) const override;

    outcome::result<Buffer> marshal(const PublicKey &key) const override;

    outcome::result<Buffer> marshal(const PrivateKey &key) const override;

    outcome::result<PublicKey> unmarshalPublicKey(
        const Buffer &key_bytes) const override;

    outcome::result<PrivateKey> unmarshalPrivateKey(
        const Buffer &key_bytes) const override;

    outcome::result<PrivateKey> import(
        boost::filesystem::path pem_path,
        std::string_view password) const override;

    Buffer pbkdf2(std::string_view password, const Buffer &salt,
                  uint64_t iterations, size_t key_size,
                  common::HashType hash) const override;

    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const override;

   private:
    /// provides methods for aes ctr 128 and 256
    aes::AesProvider aes_provider_;
    /// provides method for making digest
    hmac::HmacProvider hmac_provider_;
    /// provides keys marshaling
    marshaller::KeyMarshaller key_marshaller_;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_CRYPTO_PROVIDER_IMPL_HPP
