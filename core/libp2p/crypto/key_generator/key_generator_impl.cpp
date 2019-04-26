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

  void KeyGeneratorImpl::initialize(random::CSPRNG &random_provider) {
    constexpr size_t SEED_BYTES_COUNT = 128 * 4;  // ripple uses such number
    auto bytes = random_provider.randomBytes(SEED_BYTES_COUNT);
    // seeding random generator is required prior to calling RSA_generate_key
    // NOLINTNEXTLINE
    RAND_seed(static_cast<const void *>(bytes.toBytes()), bytes.size());
    is_initialized_ = true;
  }

  namespace detail {
    outcome::result<std::pair<Buffer, Buffer>> generate_keys(int bits) {
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
      int private_length = BIO_pending(bio_private);
      int public_length = BIO_pending(bio_public);

      std::vector<uint8_t> private_bytes(private_length, 0);
      std::vector<uint8_t> public_bytes(private_length, 0);

      BIO_read(bio_private, private_bytes.data(), private_length);
      BIO_read(bio_public, public_bytes.data(), public_length);

      return {std::move(private_bytes), std::move(public_bytes)};
    }
  }  // namespace detail

  outcome::result<KeyPair> KeyGeneratorImpl::generateRsa(
      common::RSAKeyType bits_option) const {
    OUTCOME_TRY(ensureInitialized());

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

    OUTCOME_TRY(bytes, detail::generate_keys(bits));

    auto public_key = PublicKey{{key_type, bytes.first}};
    auto private_key = PrivateKey{{key_type, bytes.second}};

    return KeyPair{std::move(public_key), std::move(private_key)};
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateEd25519() const {
    OUTCOME_TRY(ensureInitialized());

    //    EVP_PKEY *pkey = nullptr;
    //    EVP_PKEY_CTX *pctx = nullptr;
    //    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    //    EVP_PKEY_keygen_init(pctx);
    //    EVP_PKEY_keygen(pctx, &pkey);
    //    EVP_PKEY_CTX_free(pctx);
    //    PEM_write_PrivateKey(stdout, pkey, nullptr, nullptr, 0, nullptr,
    //    nullptr);
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }

  outcome::result<KeyPair> KeyGeneratorImpl::generateSecp256k1() const {
    OUTCOME_TRY(ensureInitialized());
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }

  outcome::result<PublicKey> KeyGeneratorImpl::derivePublicKey(
      const PrivateKey &private_key) const {
    return KeyGeneratorError::KEY_GENERATION_FAILED;
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

  outcome::result<void> KeyGeneratorImpl::ensureInitialized() const {
    if (is_initialized_) {
      return outcome::success();
    }
    return KeyGeneratorError::GENERATOR_NOT_INITIALIZED;
  }

  outcome::result<PrivateKey> KeyGeneratorImpl::import(
      boost::filesystem::path pem_path, std::string_view password) const {
    if (!boost::filesystem::exists(pem_path)) {
      return KeyGeneratorError::FILE_NOT_FOUND;
    }

    FILE *file = fopen(pem_path.c_str(), "r");
    auto close_file = gsl::finally([&]() { fclose(file); });

    //    RSA *PEM_read_RSAPublicKey(FILE *fp, RSA **x,
    //                               pem_password_cb *cb, void *u);
    return KeyGeneratorError::KEY_GENERATION_FAILED;
  }
}  // namespace libp2p::crypto
