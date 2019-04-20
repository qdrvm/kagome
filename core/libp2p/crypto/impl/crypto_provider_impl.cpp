/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/impl/crypto_provider_impl.hpp"

#include "libp2p/crypto/error/error.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/proto/keys.pb.h"

namespace libp2p::crypto {
  using kagome::common::Buffer;

  outcome::result<Buffer> CryptoProviderImpl::aesEncrypt(
      const common::Aes128Secret &secret, const Buffer &data) const {
    return aes_provider_.encrypt_128_ctr(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::aesEncrypt(
      const common::Aes256Secret &secret, const Buffer &data) const {
    return aes_provider_.encrypt_256_ctr(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::aesDecrypt(
      const common::Aes128Secret &secret, const Buffer &data) const {
    return aes_provider_.decrypt_128_ctr(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::aesDecrypt(
      const common::Aes256Secret &secret, const Buffer &data) const {
    return aes_provider_.decrypt_256_ctr(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::hmacDigest(common::HashType hash,
                                                         const Buffer &secret,
                                                         const Buffer &data) {
    return hmac_provider_.calculateDigest(hash, secret, data);
  }

  common::KeyPair CryptoProviderImpl::generateEd25519Keypair() const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  common::KeyPair CryptoProviderImpl::generateRSAKeypair(
      common::RSAKeyType key_type) const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  common::EphemeralKeyPair CryptoProviderImpl::generateEphemeralKeyPair(
      common::CurveType curve) const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  std::vector<common::StretchedKey> CryptoProviderImpl::keyStretcher(
      common::CipherType cipher_type, common::HashType hash_type,
      const Buffer &secret) const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  outcome::result<Buffer> CryptoProviderImpl::marshal(
      const PublicKey &key) const {
    return key_marshaler_.marshal(key);
  }

  outcome::result<Buffer> CryptoProviderImpl::marshal(
      const PrivateKey &key) const {
    return key_marshaler_.marshal(key);
  }

  outcome::result<PublicKey> CryptoProviderImpl::unmarshalPublicKey(
      const Buffer &key_bytes) const {
    return key_marshaler_.unmarshalPublicKey(key_bytes);
  }

  outcome::result<PrivateKey> CryptoProviderImpl::unmarshalPrivateKey(
      const Buffer &key_bytes) const {
    return key_marshaler_.unmarshalPrivateKey(key_bytes);
  }

  outcome::result<PrivateKey> CryptoProviderImpl::import(
      boost::filesystem::path pem_path, std::string_view password) const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  Buffer CryptoProviderImpl::pbkdf2(std::string_view password,
                                    const Buffer &salt, uint64_t iterations,
                                    size_t key_size,
                                    common::HashType hash) const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  outcome::result<PublicKey> CryptoProviderImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    // TODO(yuraz):  implement
    std::terminate();
  }
}  // namespace libp2p::crypto
