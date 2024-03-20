/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <optional>
#include <filesystem>

#include "crypto/key_store/crypto_suites.hpp"
#include "crypto/key_store/key_type.hpp"
#include "filesystem/common.hpp"
#include "utils/json_unquote.hpp"

namespace kagome::crypto {

  enum class KeyStoreError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_CRYPTO_TYPE,
    WRONG_SEED_SIZE,
    KEY_NOT_FOUND,
    BABE_ALREADY_EXIST,
    GRAN_ALREADY_EXIST,
    AUDI_ALREADY_EXIST,
    WRONG_PUBLIC_KEY,
  };

  template <typename T>
  concept Suite = requires(T t) {
    typename T::Keypair;
    typename T::PrivateKey;
    typename T::PublicKey;
    typename T::Seed;
  };

  template <Suite T>
  class KeySuiteStore {
   public:
    using Path = filesystem::path;
    using Keypair = typename T::Keypair;
    using PrivateKey = typename T::PrivateKey;
    using PublicKey = typename T::PublicKey;

    virtual ~KeySuiteStore() = default;

    /**
     * @brief generates a keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<Keypair> generateKeypair(
        KeyType key_type, std::string_view mnemonic_phrase) = 0;

    /**
     * @brief generates a keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair
     */
    virtual outcome::result<Keypair> generateKeypair(
        KeyType key_type, common::BufferView seed) = 0;

    /**
     * @brief generates a keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<Keypair> generateKeypairOnDisk(
        KeyType key_type) = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual outcome::result<Keypair> findKeypair(KeyType key_type,
                                                 const PublicKey &pk) const = 0;
    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param path path of the keypair file to look for
     * @return found key pair if exists
     */
    // virtual outcome::result<Keypair> findKeypair(
    //     KeyType key_type, const filesystem::path &path) const = 0;

    /**
     * @brief searches for public keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual outcome::result<std::vector<PublicKey>> getPublicKeys(
        KeyType key_type) const = 0;
  };

  class KeyStore {
   public:
    KeyStore(std::unique_ptr<KeySuiteStore<Sr25519Suite>> sr25519,
             std::unique_ptr<KeySuiteStore<Ed25519Suite>> ed25519,
             std::unique_ptr<KeySuiteStore<EcDsaSuite>> ecdsa)
        : sr25519_{std::move(sr25519)},
          ed25519_{std::move(ed25519)},
          ecdsa_{std::move(ecdsa)} {
      BOOST_ASSERT(sr25519_);
      BOOST_ASSERT(ed25519_);
      BOOST_ASSERT(ecdsa_);
    }

    KeySuiteStore<Sr25519Suite> &sr25519() {
      return *sr25519_;
    }

    KeySuiteStore<Ed25519Suite> &ed25519() {
      return *ed25519_;
    }

    KeySuiteStore<EcDsaSuite> &ecdsa() {
      return *ecdsa_;
    }

   private:
    outcome::result<void> scanKeyDirectory(std::filesystem::path const& dir) {
      // scan directory and collect every type-public key-seed triplet
      // for every key type
      //  for every allowed algorithm for this key type
      //    try to make a public key from the seed, if it matches the original key, add this keypair to the store
      std::error_code ec{};
      std::filesystem::directory_iterator iter{dir, ec};
      if (ec) {
        return ec;
      }
      for (auto& file: iter) {
        if (file.is_regular_file()) {
          // read file
          std::basic_string<char, std::char_traits<char>, SecureHeapAllocator<char>> content;
          if(!readFile(content, file.path())) {
            return error;
          }
          if (not content.empty() and content[0] == '"') {
            if (auto str = jsonUnquote(content)) {
              return str;
            }
            return Error::INVALID_FILE_FORMAT;
          }
          OUTCOME_TRY(decoded_parts, decodeKeyFileName(file.path().filename()));
          auto [key_type, public_key] = std::move(decoded_parts);
          SecureBuffer<> seed(content.size() / 2 - 1);
          OUTCOME_TRY(common::unhexWith0x(content, seed.begin()));
          
          (void)sr25519_->generateKeypair(key_type, seed);
        }
      }
    }

    std::unique_ptr<KeySuiteStore<Sr25519Suite>> sr25519_;
    std::unique_ptr<KeySuiteStore<Ed25519Suite>> ed25519_;
    std::unique_ptr<KeySuiteStore<EcDsaSuite>> ecdsa_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyStoreError);
