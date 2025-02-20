/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <optional>

#include "common/buffer.hpp"
#include "crypto/common.hpp"
#include "crypto/key_store/key_type.hpp"
#include "filesystem/common.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {

  /**
   * Class for operations over key storage in file system
   */
  class KeyFileStorage {
   public:
    enum class Error : uint8_t {
      WRONG_KEYFILE_NAME = 1,
      NOT_REGULAR_FILE,
      FAILED_OPEN_FILE,
      FILE_DOESNT_EXIST,
      INVALID_FILE_FORMAT,
      INCONSISTENT_KEYFILE,
    };

    using Buffer = common::Buffer;
    using Path = filesystem::path;

    /**
     * Initializes key storage at the given \param keystore_path. Creates its
     * dir if it doesn't exist, but won't create missing parent dirs
     *
     * TODO(Harrm): #603 add enforcement of key file permissions (like in SSH)
     */
    static outcome::result<std::unique_ptr<KeyFileStorage>> createAt(
        Path keystore_path);

    /**
     * Searches for a key file for the corresponding type and public key and
     * returns its content if it's a valid hex blob or mnemonic phrase json.
     */
    outcome::result<bool> searchForKey(KeyType type,
                                       common::BufferView public_key) const;

    using PhraseOrSeed = std::variant<std::string_view, BufferView>;
    /**
     * Stores the \param seed that generates the \param public_key to the key
     * storage
     */
    outcome::result<void> saveKeyPair(KeyType type,
                                      common::BufferView public_key,
                                      PhraseOrSeed phrase_or_seed) const;

   private:
    explicit KeyFileStorage(Path keystore_path);

    outcome::result<void> initialize();

    outcome::result<std::pair<KeyType, Buffer>> parseKeyFileName(
        std::string_view file_name) const;

    Path composeKeyPath(KeyType key_type, common::BufferView public_key) const;

    Path keystore_path_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyFileStorage::Error);
