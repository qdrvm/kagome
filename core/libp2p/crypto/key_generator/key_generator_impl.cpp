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
  using crypto::common::KeyType;
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
      BIO *bp_public = nullptr;
      BIO *bp_private = nullptr;

      auto cleanup = gsl::finally([&]() {
        BIO_free_all(bp_public);
        BIO_free_all(bp_private);
        RSA_free(rsa);
        BN_free(bne);
      });

      unsigned long exp = RSA_F4;

      // 1. generate rsa key
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

      // 2. save keys
      bp_private = BIO_new(BIO_s_mem());  // in-memory storage for key
      bp_public = BIO_new(BIO_s_mem());   // in-memory storage for key

      ret = PEM_write_bio_RSAPublicKey(bp_public, rsa);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      ret = PEM_write_bio_RSAPrivateKey(bp_private, rsa, nullptr, nullptr, 0,
                                        nullptr, nullptr);
      if (ret != 1) {
        return KeyGeneratorError::KEY_GENERATION_FAILED;
      }

      // 4. Get the keys are PEM formatted strings
      int pri_len = BIO_pending(bp_private);
      int pub_len = BIO_pending(bp_public);

      std::vector<uint8_t> pri_key(pri_len, 0);
      std::vector<uint8_t> pub_key(pri_len, 0);

      BIO_read(bp_private, pri_key.data(), pri_len);
      BIO_read(bp_public, pub_key.data(), pub_len);

      Buffer private_bytes(std::move(pri_key));
      Buffer public_bytes(std::move(pub_key));

      return {std::move(public_bytes), std::move(private_bytes)};
    }
  }  // namespace detail

  outcome::result<common::KeyPair> KeyGeneratorImpl::generateRsa(
      common::RSAKeyType bits_option) const {
    OUTCOME_TRY(ensureInitialized());

    int bits = 0;
    KeyType key_type;
    switch (bits_option) {
      case common::RSAKeyType::RSA1024:
        bits = 1024;
        key_type = KeyType::RSA1024;
        break;
      case common::RSAKeyType::RSA2048:
        bits = 2048;
        key_type = KeyType::RSA2048;
        break;
      case common::RSAKeyType::RSA4096:
        bits = 4096;
        key_type = KeyType::RSA4096;
        break;
    }

    // normally unreachable
    if (0 == bits) {
      return KeyGeneratorError::UNKNOWN_BITS_COUNT;
    }

    OUTCOME_TRY(bytes, detail::generate_keys(bits));

    auto public_key = std::make_shared<PublicKey>(key_type, bytes.first);
    auto private_key = std::make_shared<PrivateKey>(key_type, bytes.second);

    return common::KeyPair{std::move(public_key), std::move(private_key)};
  }

  outcome::result<common::KeyPair> KeyGeneratorImpl::generateEd25519() const {
    OUTCOME_TRY(ensureInitialized());

    //    EVP_PKEY *pkey = nullptr;
    //    EVP_PKEY_CTX *pctx = nullptr;
    //    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    //    EVP_PKEY_keygen_init(pctx);
    //    EVP_PKEY_keygen(pctx, &pkey);
    //    EVP_PKEY_CTX_free(pctx);
    //    PEM_write_PrivateKey(stdout, pkey, nullptr, nullptr, 0, nullptr,
    //    nullptr);
  }

  outcome::result<common::KeyPair> KeyGeneratorImpl::generateSecp256k1() const {
    OUTCOME_TRY(ensureInitialized());
  }

  bool generateKeys(common::RSAKeyType option) {
    size_t pri_len;  // Length of private key
    size_t pub_len;  // Length of public key
    char *pri_key;   // Private key in PEM
    char *pub_key;   // Public key in PEM

    int ret = 0;
    RSA *r = nullptr;
    BIGNUM *bne = nullptr;
    BIO *bp_public = nullptr, *bp_private = nullptr;
    int bits = 0;
    switch (option) {
      case common::RSAKeyType::RSA1024:
        bits = 1024;
        break;
      case common::RSAKeyType::RSA2048:
        bits = 2048;
        break;
      case common::RSAKeyType::RSA4096:
        bits = 4096;
        break;
    }
    unsigned long e = RSA_F4;

    EVP_PKEY *evp_pbkey = nullptr;
    EVP_PKEY *evp_pkey = nullptr;

    BIO *pbkeybio = nullptr;
    BIO *pkeybio = nullptr;

    // 1. generate rsa key
    bne = BN_new();
    ret = BN_set_word(bne, e);
    if (ret != 1) {
      goto free_all;
    }

    r = RSA_new();
    ret = RSA_generate_key_ex(r, bits, bne, nullptr);
    if (ret != 1) {
      goto free_all;
    }

    // 2. save public key
    // bp_public = BIO_new_file("public.pem", "w+");
    bp_public = BIO_new(BIO_s_mem());
    ret = PEM_write_bio_RSAPublicKey(bp_public, r);
    if (ret != 1) {
      goto free_all;
    }

    // 3. save private key
    // bp_private = BIO_new_file("private.pem", "w+");
    bp_private = BIO_new(BIO_s_mem());  // in-memory storage for key
    ret = PEM_write_bio_RSAPrivateKey(bp_private, r, nullptr, nullptr, 0,
                                      nullptr, nullptr);

    // 4. Get the keys are PEM formatted strings
    pri_len = BIO_pending(bp_private);
    pub_len = BIO_pending(bp_public);

    pri_key = (char *)malloc(pri_len + 1);
    pub_key = (char *)malloc(pub_len + 1);

    BIO_read(bp_private, pri_key, pri_len);
    BIO_read(bp_public, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    printf("\n%s\n%s\n", pri_key, pub_key);  // keys

    // verify if you are able to re-construct the keys
    pbkeybio = BIO_new_mem_buf((void *)pub_key, -1);
    if (pbkeybio == nullptr) {
      return -1;
    }
    evp_pbkey = PEM_read_bio_PUBKEY(pbkeybio, &evp_pbkey, nullptr, nullptr);
    if (evp_pbkey == nullptr) {
      char buffer[120];
      ERR_error_string(ERR_get_error(), buffer);
      printf("Error reading public key:%s\n", buffer);
    }

    pkeybio = BIO_new_mem_buf((void *)pri_key, -1);
    if (pkeybio == nullptr) {
      return -1;
    }
    evp_pkey = PEM_read_bio_PrivateKey(pkeybio, &evp_pkey, nullptr, nullptr);
    if (evp_pbkey == nullptr) {
      char buffer[120];
      ERR_error_string(ERR_get_error(), buffer);
      printf("Error reading private key:%s\n", buffer);
    }

    BIO_free(pbkeybio);
    BIO_free(pkeybio);

  // 4. free
  free_all:

    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);

    return (ret == 1);
  }

  outcome::result<PublicKey> KeyGeneratorImpl::derivePublicKey(
      const PrivateKey &private_key) const {}

  outcome::result<common::EphemeralKeyPair>
  KeyGeneratorImpl::generateEphemeralKeyPair(common::CurveType curve) const {}

  std::vector<common::StretchedKey> KeyGeneratorImpl::stretchKey(
      common::CipherType cipher_type, common::HashType hash_type,
      const kagome::common::Buffer &secret) const {}

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
  }
}  // namespace libp2p::crypto
