/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include "libp2p/crypto/error.hpp"

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <exception>
#include <iostream>

namespace libp2p::crypto {
  using crypto::common::KeyType;
  enum class RsaLength {
    RSA1024, RSA2048, RSA4096
  };

  bool generateKeys(RsaLength option) {
    size_t pri_len;  // Length of private key
    size_t pub_len;  // Length of public key
    char *pri_key;   // Private key in PEM
    char *pub_key;   // Public key in PEM

    int ret = 0;
    RSA *r = nullptr;
    BIGNUM *bne = nullptr;
    BIO *bp_public = nullptr, *bp_private = nullptr;
    int bits = 0;
    switch(option) {
      case RsaLength::RSA1024:
        bits = 1024;
        break;
      case RsaLength::RSA2048:
        bits = 2048;
        break;
      case RsaLength::RSA4096:
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
    bp_private = BIO_new(BIO_s_mem()); // in-memory storage for key
    ret = PEM_write_bio_RSAPrivateKey(bp_private, r, nullptr, nullptr, 0, nullptr, nullptr);

    // 4. Get the keys are PEM formatted strings
    pri_len = BIO_pending(bp_private);
    pub_len = BIO_pending(bp_public);

    pri_key = (char *)malloc(pri_len + 1);
    pub_key = (char *)malloc(pub_len + 1);

    BIO_read(bp_private, pri_key, pri_len);
    BIO_read(bp_public, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    printf("\n%s\n%s\n", pri_key, pub_key); // keys

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

  outcome::result<common::KeyPair> KeyGeneratorImpl::generateKeyPair(
      common::KeyType key_type) const {
//    EVP_PKEY *pkey = nullptr;
//    EVP_PKEY_CTX *pctx = nullptr;
//    switch (key_type) {
//      case KeyType::RSA1024: {
//        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_rsa, nullptr);
//      } break;
//      case KeyType::RSA2048:
//        break;
//      case KeyType::RSA4096:
//        break;
//      case KeyType::ED25519:
//        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
//        break;
//      case KeyType::SECP256K1:
//        break;
//      case KeyType::UNSPECIFIED:
//        return KeyGeneratorError::CANNOT_GENERATE_UNSPECIFIED;
//    }
//
//    if (nullptr == pctx) {
//      return KeyGeneratorError::UNKNOWN_KEY_TYPE;
//    }
//
//    EVP_PKEY_keygen_init(pctx);
//    EVP_PKEY_keygen(pctx, &pkey);
//    EVP_PKEY_CTX_free(pctx);
//    PEM_write_PrivateKey(stdout, pkey, nullptr, nullptr, 0, nullptr, nullptr);
//
//    return {};
  }

  outcome::result<PublicKey> KeyGeneratorImpl::derivePublicKey(
      const PrivateKey &private_key) const {}

  outcome::result<common::EphemeralKeyPair>
  KeyGeneratorImpl::generateEphemeralKeyPair(common::CurveType curve) const {}

  std::vector<common::StretchedKey> KeyGeneratorImpl::keyStretcher(
      common::CipherType cipher_type, common::HashType hash_type,
      const kagome::common::Buffer &secret) const {}
}  // namespace libp2p::crypto
