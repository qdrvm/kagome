/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_FILE_STORAGE_HPP
#define KAGOME_CRYPTO_KEY_FILE_STORAGE_HPP

#include <vector>

#include <boost/filesystem.hpp>
#include <optional>

#include "common/buffer.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {

  /**
   * Class for operations over key storage in file system
   */
  class KeyFileStorage {
   public:
    enum class Error {
      WRONG_KEYFILE_NAME = 1,
      NOT_REGULAR_FILE,
      FAILED_OPEN_FILE,
      FILE_DOESNT_EXIST,
      INVALID_FILE_FORMAT,
      INCONSISTENT_KEYFILE,
      KEYS_PATH_IS_NOT_DIRECTORY,
      FAILED_CREATE_KEYS_DIRECTORY
    };

    using Buffer = common::Buffer;
    using Path = boost::filesystem::path;

    /**
     * Initializes key storage at the given \param keystore_path. Creates its
     * dir if it doesn't exist, but won't create missing parent dirs
     *
     * TODO(Harrm): add enforcement of key file permissions (like in SSH)
     */
    static outcome::result<std::unique_ptr<KeyFileStorage>> createAt(
        Path keystore_path);

    /**
     * Collects all public keys of the given type from the key storage
     */
    outcome::result<std::vector<Buffer>> collectPublicKeys(
        KeyTypeId type) const;

    /**
     * Searches for a key file for the corresponding type and public key and
     * returns its content if it's a valid hex blob or mnemonic phrase json.
     */
    outcome::result<std::optional<std::string>> searchForPhrase(
        KeyTypeId type, gsl::span<const uint8_t> public_key_bytes) const;

    /**
     * Stores the \param seed that generates the \param public_key to the key
     * storage
     */
    outcome::result<void> saveKeyPair(KeyTypeId type,
                                      gsl::span<const uint8_t> public_key,
                                      gsl::span<const uint8_t> seed) const;

    /**
     * Save key as hex to the specific path.
     * Used when --node-key-file flag is specified.
     * @param key_bytes - key contents to save
     * @param file_path - user-provided path to create the file
     * @return an error if any
     */
    outcome::result<void> saveKeyHexAtPath(gsl::span<const uint8_t> private_key,
                                           const Path &path) const;

   private:
    explicit KeyFileStorage(Path keystore_path);

    outcome::result<void> initialize();

    outcome::result<std::pair<KeyTypeId, Buffer>> parseKeyFileName(
        std::string_view file_name) const;

    Path composeKeyPath(KeyTypeId key_type,
                        gsl::span<const uint8_t> public_key) const;

    Path keystore_path_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyFileStorage::Error);

#endif  // KAGOME_CRYPTO_KEY_FILE_STORAGE_HPP
