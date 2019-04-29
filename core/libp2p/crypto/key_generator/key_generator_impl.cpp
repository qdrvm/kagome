/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"

#include <stdio.h>
#include <exception>
#include <iostream>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/random/csprng.hpp"

namespace libp2p::crypto {
  using kagome::common::Buffer;

  KeyGeneratorImpl::KeyGeneratorImpl(random::CSPRNG &random_provider)
      : random_provider_(random_provider) {
    initialize();
  }

  void KeyGeneratorImpl::initialize() {
    constexpr size_t kSeedBytesCount = 128 * 4;  // ripple uses such number
    auto bytes = random_provider_.randomBytes(kSeedBytesCount);
    // seeding random generator is required prior to calling RSA_generate_key
    // NOLINTNEXTLINE
    RAND_seed(static_cast<const void *>(bytes.toBytes()), bytes.size());
  }

  namespace detail {
    /**
     * @brief makes Buffer from BIO
     * @param bio data
     * @return Buffer instance containg data
     */
    Buffer bio2buffer(BIO *bio) {
      auto length = BIO_pending(bio);
      std::vector<uint8_t> bytes(length, 0);
      BIO_read(bio, bytes.data(), length);
      return Buffer(std::move(bytes));
    }

    outcome::result<Buffer> getPrivateBytes(EVP_PKEY *pkey) {
      BIO *bio = BIO_new(BIO_s_mem());

      if (1
          != PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr,
                                      nullptr)) {
        return KeyGeneratorError::GET_KEY_BYTES_FAILED;
      }

      // 3. Get keys
      return bio2buffer(bio);
    }

    outcome::result<Buffer> getPublicBytes(EVP_PKEY *pkey) {
      BIO *bio = BIO_new(BIO_s_mem());

      if (1 != PEM_write_bio_PUBKEY(bio, pkey)) {
        return KeyGeneratorError::GET_KEY_BYTES_FAILED;
      }

      // 3. Get keys
      return bio2buffer(bio);
    }

    /**
     * @brief generates private and public rsa key contents
     * @param bits rsa key bits count
     * @return pair of private and public key content
     */
    outcome::result<std::pair<Buffer, Buffer>> generateRsaKeys(int bits) {
      int ret = 0;
      RSA *rsa = nullptr;
      BIGNUM *bne = nullptr;
      BIO *bio_public = nullptr;
      BIO *bio_private = nullptr;

      // clean up automatically
      auto cleanup = gsl::finally([&]() {
        BIO_free_all(bio_public);
        BIO_free_all(bio_private);
        RSA_free(rsa);
        BN_free(bne);
      });

      unsigned long exp = RSA_F4;

      // 1. generate rsa state
      bne = BN_new();
      ret = BN_set_word(bne, exp);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      rsa = RSA_new();
      ret = RSA_generate_key_ex(rsa, bits, bne, nullptr);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      // 2. save keys to memory
      bio_private = BIO_new(BIO_s_mem());
      bio_public = BIO_new(BIO_s_mem());

      ret = PEM_write_bio_RSAPublicKey(bio_public, rsa);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      ret = PEM_write_bio_RSAPrivateKey(bio_private, rsa, nullptr, nullptr, 0,
                                        nullptr, nullptr);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      // 3. Get keys
      auto private_buffer = bio2buffer(bio_private);
      auto public_buffer = bio2buffer(bio_public);

      return {std::move(private_buffer), std::move(public_buffer)};
    }
  }  // namespace detail

  outcome::result<KeyPair> KeyGeneratorImpl::generateRsa(
      common::RSAKeyType bits_option) const {
    int bits = 0;
    Key::Type key_type;
    switch (bits_option) {
      case common::RSAKeyType::RSA1024:
        bits = 1024;
        key_type = Key::Type::RSA1024;
        break;
      case common::RSAKeyType::RSA2048:
        bits = 2048;
        key_type = Key::Type::RSA2048;
        break;
      case common::RSAKeyType::RSA4096:
        bits = 4096;
        key_type = Key::Type::RSA4096;
        break;
    }

    // normally unreachable
    if (0 == bits) {
      return KeyGeneratorError::INCORRECT_BITS_COUNT;
    }

    OUTCOME_TRY(keys, detail::generateRsaKeys(bits));

    auto public_key = PublicKey{{key_type, std::move(keys.first)}};
    auto private_key = PrivateKey{{key_type, std::move(keys.second)}};

    return KeyPair{std::move(public_key), std::move(private_key)};
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateEd25519() const {
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *pctx = nullptr;
    BIO *bio_public = nullptr;
    BIO *bio_private = nullptr;

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);

    auto cleanup = gsl::finally([&]() {
      EVP_PKEY_free(pkey);
      EVP_PKEY_CTX_free(pctx);
      BIO_free_all(bio_public);
      BIO_free_all(bio_private);
    });

    auto ret = EVP_PKEY_keygen_init(pctx);
    if (ret != 1) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    ret = EVP_PKEY_keygen(pctx, &pkey);
    if (ret != 1) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // 2. save keys to memory
    bio_private = BIO_new(BIO_s_mem());
    bio_public = BIO_new(BIO_s_mem());

    if (1 != PEM_write_bio_PUBKEY(bio_public, pkey)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    if (1
        != PEM_write_bio_PrivateKey(bio_private, pkey, nullptr, nullptr, 0,
                                    nullptr, nullptr)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // 3. Get keys
    auto private_buffer = detail::bio2buffer(bio_private);
    auto public_buffer = detail::bio2buffer(bio_public);

    auto public_key = PublicKey{{Key::Type::ED25519, std::move(public_buffer)}};
    auto private_key =
        PrivateKey{{Key::Type::ED25519, std::move(private_buffer)}};

    return KeyPair{std::move(public_key), std::move(private_key)};
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateSecp256k1() const {
    constexpr size_t bytes_length = 32;  // need to get 32 random bytes
    auto random_bytes = random_provider_.randomBytes(bytes_length);

    EC_KEY *key = EC_KEY_new();
    if (nullptr == key) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (nullptr == group) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    if (1 != EC_KEY_set_group(key, group)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    if (1 != EC_KEY_generate_key(key)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // 2. save keys to memory
    BIO *bio_public = BIO_new(BIO_s_mem());
    if (1 != PEM_write_bio_EC_PUBKEY(bio_public, key)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    BIO *bio_private = BIO_new(BIO_s_mem());
    if (1
        != PEM_write_bio_ECPrivateKey(bio_private, key, nullptr, nullptr, 0,
                                      nullptr, nullptr)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // 3. Get keys
    auto public_bytes = detail::bio2buffer(bio_public);
    auto private_bytes = detail::bio2buffer(bio_private);

    auto public_key =
        PublicKey{{Key::Type::SECP256K1, std::move(public_bytes)}};
    auto private_key =
        PrivateKey{{Key::Type::SECP256K1, std::move(private_bytes)}};

    return KeyPair{std::move(public_key), std::move(private_key)};
  }

  namespace {
    outcome::result<PublicKey> deriveRsaPublicKey(
        const PrivateKey &private_key) {
      auto &buffer = private_key.data;
      BIO *bio = BIO_new_mem_buf((void *)(buffer.toBytes()), buffer.size());
      if (nullptr == bio) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto free_bio = gsl::finally([bio]() { BIO_free_all(bio); });

      RSA *rsa = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
      if (nullptr == rsa) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto free_rsa = gsl::finally([rsa]() { RSA_free(rsa); });

      EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
      OUTCOME_TRY(bytes, detail::getPublicBytes(pkey));

      BIO *bio_public = BIO_new(BIO_s_mem());
      auto free_bio_public =
          gsl::finally([bio_public]() { BIO_free(bio_public); });

      if (1 != PEM_write_bio_RSAPublicKey(bio_public, rsa)) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }

      auto public_bytes = detail::bio2buffer(bio_public);
      return PublicKey{{private_key.type, std::move(public_bytes)}};
    }
  }  // namespace

  outcome::result<PublicKey> KeyGeneratorImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA1024:
      case Key::Type::RSA2048:
      case Key::Type::RSA4096:
        return deriveRsaPublicKey(private_key);
      case Key::Type::ED25519:
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      case Key::Type::SECP256K1:
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
    }

    return KeyGeneratorError::WRONG_KEY_TYPE;
  }

  outcome::result<EphemeralKeyPair> KeyGeneratorImpl::generateEphemeralKeyPair(
      common::CurveType curve) const {
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }

  std::vector<StretchedKey> KeyGeneratorImpl::stretchKey(
      common::CipherType cipher_type, common::HashType hash_type,
      const kagome::common::Buffer &secret) const {
    return {};
  }

  outcome::result<EVP_PKEY *> loadKey(const boost::filesystem::path &pem_path,
                                      std::string_view password) {
    if (!boost::filesystem::exists(pem_path)) {
      return KeyGeneratorError::FILE_NOT_FOUND;
    }

    BIO *key = nullptr;
    EVP_PKEY *pkey = nullptr;

    key = BIO_new(BIO_s_file());
    if (1 != BIO_read_filename(key, pem_path.c_str())) {
      return KeyGeneratorError::FAILED_TO_READ_FILE;
    }

    std::vector<char> password_str(password.length() + 1, 0);
    std::copy(password.begin(), password.end(), password_str.begin());

    pkey = PEM_read_bio_PrivateKey(key, nullptr, nullptr,
                                   static_cast<void *>(password_str.data()));
    if (nullptr == pkey) {
      return KeyGeneratorError::FAILED_TO_READ_FILE;
    }

    return pkey;
  }

  outcome::result<PrivateKey> KeyGeneratorImpl::importKey(
      const boost::filesystem::path &pem_path,
      std::string_view password) const {
    OUTCOME_TRY(pkey, loadKey(pem_path, password));

    auto free_key =
        gsl::finally([pkey]() { EVP_PKEY_free(pkey); });  // free key

    switch (EVP_PKEY_base_id(pkey)) {
      case EVP_PKEY_RSA: {
        RSA *rsa = EVP_PKEY_get1_RSA(pkey);
        if (nullptr == rsa) {
          return KeyGeneratorError::WRONG_KEY_TYPE;
        }

        OUTCOME_TRY(buffer, detail::getPrivateBytes(pkey));
        auto bits = RSA_security_bits(rsa);
        switch (bits) {
          case 1024:
            return PrivateKey{{Key::Type::RSA1024, std::move(buffer)}};
          case 2048:
            return PrivateKey{{Key::Type::RSA2048, std::move(buffer)}};
          case 4096:
            return PrivateKey{{Key::Type::RSA4096, std::move(buffer)}};
          default:
            return KeyGeneratorError::INCORRECT_BITS_COUNT;
        }
      }
      case EVP_PKEY_ED25519: {
        OUTCOME_TRY(buffer, detail::getPrivateBytes(pkey));
        return PrivateKey{{Key::Type::ED25519, std::move(buffer)}};
      }
      case NID_secp256k1: {
        // not sure about it
        OUTCOME_TRY(buffer, detail::getPrivateBytes(pkey));
        return PrivateKey{{Key::Type::SECP256K1, std::move(buffer)}};
      }
      default:
        return KeyGeneratorError::WRONG_KEY_TYPE;
    }
  }
}  // namespace libp2p::crypto
