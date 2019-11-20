/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/crypto_key_reader/crypto_key_reader_util.hpp"

#include <openssl/pem.h>

#include "application/impl/key_storage_error.hpp"
#include "common/hexutil.hpp"
#include "crypto/ed25519_types.hpp"

namespace kagome::application {

  namespace detail {
    outcome::result<common::Buffer> readEd25519PrivKeyFromPEM(
        const boost::filesystem::path &file) {
      std::unique_ptr<FILE, std::function<void(FILE *)>> f(
          fopen(file.string().c_str(), "r"), fclose);
      if (f == nullptr) {
        return KeyStorageError::FILE_READ_ERROR;
      }
      EVP_PKEY *key = nullptr;
      EVP_PKEY *result =
          PEM_read_PrivateKey(f.get(), &key, nullptr, nullptr);
      auto free_pkey = gsl::finally([key] { EVP_PKEY_free(key); });
      if (result == nullptr) {
        return KeyStorageError::PRIVATE_KEY_READ_ERROR;
      }
      size_t len = crypto::constants::ed25519::PRIVKEY_SIZE;
      common::Buffer bytes(len, 0);
      if (EVP_PKEY_get_raw_private_key(key, bytes.data(), &len) != 1) {
        return KeyStorageError::MALFORMED_KEY;
      }
      if (len != bytes.size()) {
        return KeyStorageError::MALFORMED_KEY;
      }

      return bytes;
    }

    outcome::result<common::Buffer> readRsaPrivKeyFromPEM(
        const boost::filesystem::path &file) {
      // TODO(Harrm): make it work, yields an error on public key validation
      std::unique_ptr<FILE, std::function<void(FILE *)>> f(
          fopen(file.string().c_str(), "r"), fclose);
      if (!f) {
        return KeyStorageError::FILE_READ_ERROR;
      }

      RSA *key = nullptr;
      RSA *result = PEM_read_RSAPrivateKey(f.get(), &key, nullptr, nullptr);
      auto free_pkey = gsl::finally([key] { RSA_free(key); });
      if (result == nullptr) {
        return KeyStorageError::PRIVATE_KEY_READ_ERROR;
      }
      uint8_t *raw_bytes = nullptr;
      auto r = i2d_RSAPrivateKey(key, &raw_bytes);
      auto free_bytes = gsl::finally([raw_bytes] { OPENSSL_free(raw_bytes); });
      if (r < 0) {
        return KeyStorageError::MALFORMED_KEY;
      }
      common::Buffer bytes(RSA_size(key), 0);
      gsl::span<uint8_t> span_bytes(raw_bytes, bytes.size());
      std::copy(span_bytes.begin(), span_bytes.end(), bytes.begin());
      return bytes;
    }
  }  // namespace detail

  outcome::result<common::Buffer> readKeypairFromHexFile(
      const boost::filesystem::path &filepath) {
    std::ifstream file(filepath.string());
    if (!file) {
      return KeyStorageError::FILE_READ_ERROR;
    }
    std::string hex;
    file >> hex;
    OUTCOME_TRY(bytes, common::unhex(hex));
    return common::Buffer(std::move(bytes));
  }

  outcome::result<common::Buffer> readPrivKeyFromHexFile(
      const boost::filesystem::path &filepath) {
    std::ifstream file(filepath.string());
    if (!file) {
      return KeyStorageError::FILE_READ_ERROR;
    }
    std::string hex;
    file >> hex;
    OUTCOME_TRY(bytes, common::unhex(hex));
    return common::Buffer(std::move(bytes));
  }

  outcome::result<common::Buffer> readPrivKeyFromPEM(
      const boost::filesystem::path &file, libp2p::crypto::Key::Type type) {
    using Type = libp2p::crypto::Key::Type;
    switch (type) {
      case Type::Ed25519:
        return detail::readEd25519PrivKeyFromPEM(file);
      case Type::RSA:
      case Type::Secp256k1:
      case Type::ECDSA:
      case Type::UNSPECIFIED:
        return KeyStorageError::UNSUPPORTED_KEY_TYPE;
    }
    return KeyStorageError::UNSUPPORTED_KEY_TYPE;
  }
}  // namespace kagome::application
