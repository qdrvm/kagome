/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"

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
    Buffer bio2buffer(BIO *bio) {
      auto length = BIO_pending(bio);
      std::vector<uint8_t> bytes(length, 0);
      BIO_read(bio, bytes.data(), length);
      return Buffer(std::move(bytes));
    }

    outcome::result<EVP_PKEY *> loadKey(const boost::filesystem::path &pem_path,
                                        std::string_view password) {
      if (!boost::filesystem::exists(pem_path)) {
        return KeyGeneratorError::FILE_NOT_FOUND;
      }

      BIO *bio = nullptr;
      bio = BIO_new(BIO_s_file());
      if (1 != BIO_read_filename(bio, pem_path.c_str())) {
        return KeyGeneratorError::FAILED_TO_READ_FILE;
      }
      auto free_key = gsl::finally([bio]() { BIO_free_all(bio); });

      std::vector<char> password_str(password.length() + 1, 0);
      std::copy(password.begin(), password.end(), password_str.begin());

      auto *password_ptr = password_str.size() > 1
          ? static_cast<void *>(password_str.data())
          : nullptr;

      EVP_PKEY *pkey =
          PEM_read_bio_PrivateKey(bio, nullptr, nullptr, password_ptr);
      if (nullptr == pkey) {
        return KeyGeneratorError::FAILED_TO_READ_FILE;
      }

      return pkey;
    }

    outcome::result<PublicKey> derivePublicKey(const PrivateKey &key) {
      auto &buffer = key.data;
      BIO *bio_private =
          BIO_new_mem_buf((void *)(buffer.toBytes()), buffer.size());
      if (nullptr == bio_private) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }
      auto free_bio =
          gsl::finally([bio_private]() { BIO_free_all(bio_private); });

      EVP_PKEY *pkey =
          PEM_read_bio_PrivateKey(bio_private, nullptr, nullptr, nullptr);
      if (nullptr == pkey) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }

      BIO *bio_public = BIO_new(BIO_s_mem());
      auto free_bio_public =
          gsl::finally([bio_public]() { BIO_free_all(bio_public); });

      if (1 != PEM_write_bio_PUBKEY(bio_public, pkey)) {
        return KeyGeneratorError::KEY_DERIVATION_FAILED;
      }

      auto public_buffer = detail::bio2buffer(bio_public);
      return PublicKey{{key.type, std::move(public_buffer)}};
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
    auto free_key = gsl::finally([key]() { EC_KEY_free(key); });

    EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (nullptr == group) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    auto free_group = gsl::finally([group]() { EC_GROUP_free(group); });

    if (1 != EC_KEY_set_group(key, group)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
    if (1 != EC_KEY_generate_key(key)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }

    // 2. save keys to memory
    BIO *bio_public = BIO_new(BIO_s_mem());
    BIO *bio_private = BIO_new(BIO_s_mem());
    auto free_bio = gsl::finally([bio_private, bio_public]() {
      BIO_free_all(bio_private);
      BIO_free_all(bio_public);
    });

    if (1 != PEM_write_bio_EC_PUBKEY(bio_public, key)) {
      return KeyGeneratorError::KEY_GENERATION_FAILED;
    }
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

  outcome::result<PublicKey> KeyGeneratorImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    switch (private_key.type) {
      case Key::Type::RSA1024:
      case Key::Type::RSA2048:
      case Key::Type::RSA4096:
      case Key::Type::ED25519:
      case Key::Type::SECP256K1:
        return detail::derivePublicKey(private_key);
      case Key::Type::UNSPECIFIED:
        return KeyGeneratorError::WRONG_KEY_TYPE;
    }
    return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
  }

  outcome::result<EphemeralKeyPair> KeyGeneratorImpl::generateEphemeralKeyPair(
      common::CurveType curve) const {
    // TODO(yuraz): pre-140 implement
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }

  std::vector<StretchedKey> KeyGeneratorImpl::stretchKey(
      common::CipherType cipher_type, common::HashType hash_type,
      const kagome::common::Buffer &secret) const {
    // TODO(yuraz): pre-140 implement
    return {};
  }

  outcome::result<PrivateKey> KeyGeneratorImpl::importKey(
      const boost::filesystem::path &pem_path,
      std::string_view password) const {
    constexpr int kSECP256k1_TYPE = 408;  // not sure it is right way
    OUTCOME_TRY(pkey, detail::loadKey(pem_path, password));

    auto free_key =
        gsl::finally([&pkey]() { EVP_PKEY_free(pkey); });  // free key

    BIO *bio = BIO_new(BIO_s_mem());
    auto free_bio = gsl::finally([bio]() { BIO_free(bio); });
    if (1
        != PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr,
                                    nullptr)) {
      return KeyGeneratorError::GET_KEY_BYTES_FAILED;
    }
    auto buffer = detail::bio2buffer(bio);

    auto key_type = EVP_PKEY_base_id(pkey);
    switch (key_type) {
      case EVP_PKEY_RSA: {
        RSA *rsa = EVP_PKEY_get1_RSA(pkey);
        if (nullptr == rsa) {
          return KeyGeneratorError::WRONG_KEY_TYPE;
        }

        auto bits = RSA_bits(rsa);
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
        return PrivateKey{{Key::Type::ED25519, std::move(buffer)}};
      }

      case kSECP256k1_TYPE: {
        EC_KEY *ec_key = EVP_PKEY_get1_EC_KEY(pkey);
        if (nullptr == ec_key) {
          return KeyGeneratorError::FAILED_TO_READ_FILE;
        }
        auto free_ec_key = gsl::finally([&ec_key]() { EC_KEY_free(ec_key); });

        return PrivateKey{{Key::Type::SECP256K1, std::move(buffer)}};
      }
      default:
        return KeyGeneratorError::UNSUPPORTED_KEY_TYPE;
    }
  }
}  // namespace libp2p::crypto
