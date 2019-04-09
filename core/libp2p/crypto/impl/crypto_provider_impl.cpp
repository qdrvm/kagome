/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/impl/crypto_provider_impl.hpp"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "libp2p/crypto/private_key.hpp"

#include "libp2p/crypto/proto/keys.pb.h"

#include "libp2p/crypto/error.hpp"

namespace libp2p::crypto {
  using kagome::common::Buffer;

  namespace {
    /**
     * @brief converts common::KeyType to proto::KeyType
     * @param key_type common key type value
     * @return proto key type value
     */
    outcome::result<proto::KeyType> marshalKeyType(common::KeyType key_type) {
      switch (key_type) {
        case common::KeyType::kUnspecified:
          return proto::KeyType::kUnspecified;
        case common::KeyType::kRSA1024:
          return proto::KeyType::kRSA1024;
        case common::KeyType::kRSA2048:
          return proto::KeyType::kRSA2048;
        case common::KeyType::kRSA4096:
          return proto::KeyType::kRSA4096;
        case common::KeyType::kED25519:
          return proto::KeyType::kED25519;
        default:
          break;
      }

      return CryptoProviderError::kUnknownKeyType;
    }

    /**
     * @brief converts proto::KeyType to common::KeyType
     * @param key_type proto key type value
     * @return common key type value
     */
    outcome::result<common::KeyType> unmarshalKeyType(proto::KeyType key_type) {
      switch (key_type) {
        case proto::KeyType::kUnspecified:
          return common::KeyType::kUnspecified;
        case proto::KeyType::kRSA1024:
          return common::KeyType::kRSA1024;
        case proto::KeyType::kRSA2048:
          return common::KeyType::kRSA2048;
        case proto::KeyType::kRSA4096:
          return common::KeyType::kRSA4096;
        default:
          break;
      }

      return CryptoProviderError::kUnknownKeyType;
    }
  }  // namespace

  void CryptoProviderImpl::initializeOpenSSL() {
    // Load the human readable error strings for libcrypto
    ERR_load_crypto_strings();

    // Load all digest and cipher algorithms
    OpenSSL_add_all_algorithms();  // NOLINT

    // Load config file, and other important initialisation
    // OPENSSL_config(nullptr); // deprecated
  }

  CryptoProviderImpl::CryptoProviderImpl() {
    initializeOpenSSL();
  }

  outcome::result<Buffer> CryptoProviderImpl::aesEncrypt(
      const common::Aes128Secret &secret, const Buffer &data) const {
    return aesCrypt_.encrypt128(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::aesEncrypt(
      const common::Aes256Secret &secret, const Buffer &data) const {
    return aesCrypt_.encrypt256(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::aesDecrypt(
      const common::Aes128Secret &secret, const Buffer &data) const {
    return aesCrypt_.decrypt128(secret, data);
  }

  outcome::result<Buffer> CryptoProviderImpl::aesDecrypt(
      const common::Aes256Secret &secret, const Buffer &data) const {
    return aesCrypt_.decrypt256(secret, data);
  }

  Buffer CryptoProviderImpl::hmacDigest(common::HashType hash,
                                        const Buffer &secret,
                                        const Buffer &data) {
    const evp_md_st *evp_md = nullptr;
    switch (hash) {
      case common::HashType::kSHA1:
        evp_md = EVP_sha1();
        break;
      case common::HashType::kSHA256:
        evp_md = EVP_sha256();
        break;
      case common::HashType::kSHA512:
        evp_md = EVP_sha512();
        break;
      default:
        break;
    }

    uint8_t *result = HMAC(evp_md, secret.toBytes(), secret.size(),
                           data.toBytes(), data.size(), nullptr, nullptr);

    delete[] result;

    // TODO(yuraz):  implement
    std::terminate();
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
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.getType()));
    proto_key.set_key_type(key_type);

    proto_key.set_key_value(key.getBytes().toBytes(), key.getBytes().size());

    auto string = proto_key.SerializeAsString();
    Buffer out;
    out.put(proto_key.SerializeAsString());

    return out;
  }

  outcome::result<Buffer> CryptoProviderImpl::marshal(
      const PrivateKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.getType()));
    proto_key.set_key_type(key_type);
    proto_key.set_key_value(key.getBytes().toBytes(), key.getBytes().size());

    auto string = proto_key.SerializeAsString();
    Buffer out;
    out.put(proto_key.SerializeAsString());

    return out;
  }

  outcome::result<PublicKey> CryptoProviderImpl::unmarshalPublicKey(
      const Buffer &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.toBytes(), key_bytes.size())) {
      return CryptoProviderError::kFailedToUnmarshalData;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));

    Buffer key_value;
    key_value.put(proto_key.key_value());
    return PublicKey(key_type, key_value);
  }

  outcome::result<PrivateKey> CryptoProviderImpl::unmarshalPrivateKey(
      const Buffer &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.toBytes(), key_bytes.size())) {
      return CryptoProviderError::kFailedToUnmarshalData;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));
    Buffer key_value;
    key_value.put(proto_key.key_value());

    return PrivateKey(key_type, key_value);
  }

  outcome::result<PrivateKey> CryptoProviderImpl::import(
      boost::filesystem::path pem_path, std::string_view password) const {
    // TODO(yuraz):  implement
    std::terminate();
  }

  Buffer CryptoProviderImpl::randomBytes(size_t number) const {
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
