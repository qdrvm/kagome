/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP
#define KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP

#include "application/key_storage.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include "common/buffer.hpp"

namespace kagome::application {

  class LocalKeyStorage : public AKeyStorage {
   public:
    enum class Error {
      INVALID_KEYSTORE_PATH = 1
    };

    static outcome::result<std::shared_ptr<LocalKeyStorage>> create(
        const boost::filesystem::path &keystore_path);

    ~LocalKeyStorage() override = default;

    crypto::Sr25519Keypair getLocalSr25519Keypair() const override;
    crypto::Ed25519Keypair getLocalEd25519Keypair() const override;
    libp2p::crypto::KeyPair getP2PKeypair() const override;

   private:
    LocalKeyStorage() = default;

    /**
     * Loads a keystore from the provided file
     */
    outcome::result<void> loadFromJson(const boost::filesystem::path &file_path);

    outcome::result<void> loadSr25519Keys(
        const boost::property_tree::ptree &tree);
    outcome::result<void> loadEd25519Keys(
        const boost::property_tree::ptree &tree);
    outcome::result<void> loadP2PKeys(const boost::property_tree::ptree &tree);

    crypto::Sr25519Keypair sr_25519_keypair_;
    crypto::Ed25519Keypair ed_25519_keypair_;
    libp2p::crypto::KeyPair p2p_keypair_;
  };

}  // namespace kagome::application

OUTCOME_HPP_DECLARE_ERROR(kagome::application, LocalKeyStorage::Error);

#endif  // KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP
