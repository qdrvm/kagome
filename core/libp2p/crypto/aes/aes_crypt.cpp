/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/aes/aes_crypt.hpp"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <gsl/gsl>

#include "libp2p/crypto/error.hpp"

namespace libp2p::crypto::aes {

  using kagome::common::Buffer;
  using libp2p::crypto::common::Aes128Secret;
  using libp2p::crypto::common::Aes256Secret;

  outcome::result<Buffer> aesEncrypt(const Buffer &data,
                                     gsl::span<const uint8_t> key,
                                     gsl::span<const uint8_t> iv,
                                     const EVP_CIPHER *cipher) {
    assert(nullptr != cipher && "cipher must not be null");
    if (nullptr == cipher) {
      return MiscError::kWrongArgumentValue;
    }

    EVP_CIPHER_CTX *ctx = nullptr;
    int len = 0;
    int cipher_len = 0;
    const auto *plain_text = data.toBytes();
    const auto plain_len = data.size();

    auto block_size = EVP_CIPHER_block_size(cipher);
    auto required_length = plain_len + block_size - 1;

    // TODO(yuraz): add check for key and iv sizes
    // TODO(yuraz): they must correlate with block_size

    std::cout << __FUNCTION__ << std::endl;
    std::cout << "block_size = " << block_size << std::endl;
    std::cout << "span size = " << key.size() << std::endl;

    std::vector<uint8_t> cipher_text;
    cipher_text.resize(required_length);

    // Create and initialise the context
    if (!(ctx = EVP_CIPHER_CTX_new())) {
      return OpenSslError::kFailedInitializeContext;
    }

    // clean-up context at exit from function
    auto clean_at_exit = gsl::finally([ctx]() { EVP_CIPHER_CTX_free(ctx); });

    // Initialise the encryption operation.
    if (1 != EVP_EncryptInit_ex(ctx, cipher, nullptr, key.data(), iv.data())) {
      return OpenSslError::kFailedInitializeOperation;
    }

    // Provide the message to be encrypted, and obtain the encrypted output.
    if (1
        != EVP_EncryptUpdate(ctx, cipher_text.data(), &len, plain_text,
                             plain_len)) {
      return OpenSslError::kFailedEncryptUpdate;
    }

    cipher_len = len;

    // Finalise the encryption. Further cipher text bytes
    // may be written at this stage.
    if (1 != EVP_EncryptFinal_ex(ctx, cipher_text.data() + len, &len)) {
      return OpenSslError::kFailedEncryptFinalize;
    }

    cipher_len += len;
    cipher_text.resize(cipher_len);

    return Buffer(std::move(cipher_text));
  }

  outcome::result<Buffer> aesDecrypt(const Buffer &data,
                                     gsl::span<const uint8_t> key,
                                     gsl::span<const uint8_t> iv,
                                     const EVP_CIPHER *cipher) {
    assert(nullptr != cipher && "cipher must not be null");
    if (nullptr == cipher) {
      return MiscError::kWrongArgumentValue;
    }

    // TODO(yuraz): add check for key and iv sizes
    // TODO(yuraz): they must correlate with block_size

    EVP_CIPHER_CTX *ctx = nullptr;

    int len = 0;
    int plain_len = 0;
    const auto *cipher_text = data.toBytes();
    const auto cipher_len = data.size();

    std::vector<uint8_t> plain_text;
    plain_text.resize(cipher_len);

    // Create and initialise the context
    if (!(ctx = EVP_CIPHER_CTX_new())) {
      return OpenSslError::kFailedInitializeContext;
    }

    // clean-up context at exit from function
    auto clean_at_exit = gsl::finally([ctx]() { EVP_CIPHER_CTX_free(ctx); });

    // Initialise the decryption operation.
    if (1 != EVP_DecryptInit_ex(ctx, cipher, nullptr, key.data(), iv.data())) {
      return OpenSslError::kFailedInitializeOperation;
    }

    // Provide the message to be decrypted, and obtain the plaintext output.
    if (1
        != EVP_DecryptUpdate(ctx, plain_text.data(), &len, cipher_text,
                             cipher_len)) {
      return OpenSslError::kFailedDecryptUpdate;
    }

    plain_len = len;

    // Finalise the decryption. Further plaintext bytes
    // may be written at this stage.
    if (1 != EVP_DecryptFinal_ex(ctx, plain_text.data() + len, &len)) {
      return OpenSslError::kFailedDecryptFinalize;
    }

    plain_len += len;

    plain_text.resize(plain_len);

    return Buffer(std::move(plain_text));
  }

  outcome::result<Buffer> AesCrypt::encrypt128(const Aes128Secret &secret,
                                               const Buffer &data) const {
    auto key_span = gsl::make_span(secret.key, 8);
    auto iv_span = gsl::make_span(secret.iv, 8);

    return aesEncrypt(data, key_span, iv_span, EVP_aes_128_cbc());
  }

  outcome::result<Buffer> AesCrypt::encrypt256(const Aes256Secret &secret,
                                               const Buffer &data) const {
    auto key_span = gsl::make_span(secret.key, 16);
    auto iv_span = gsl::make_span(secret.iv, 16);

    return aesEncrypt(data, key_span, iv_span, EVP_aes_256_cbc());
  }

  outcome::result<Buffer> AesCrypt::decrypt128(const Aes128Secret &secret,
                                               const Buffer &data) const {
    auto key_span = gsl::make_span(secret.key, 8);
    auto iv_span = gsl::make_span(secret.iv, 8);

    return aesDecrypt(data, key_span, iv_span, EVP_aes_128_cbc());
  }

  outcome::result<Buffer> AesCrypt::decrypt256(const Aes256Secret &secret,
                                               const Buffer &data) const {
    auto key_span = gsl::make_span(secret.key, 16);
    auto iv_span = gsl::make_span(secret.iv, 16);

    return aesDecrypt(data, key_span, iv_span, EVP_aes_256_cbc());
  }

}  // namespace libp2p::crypto::detail
