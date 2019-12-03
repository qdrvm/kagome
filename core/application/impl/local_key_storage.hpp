/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP
#define KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP

#include <memory>
//
#include <boost/filesystem.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_validator.hpp>
//
#include "application/key_storage.hpp"
#include "common/buffer.hpp"

namespace kagome::application {

  class LocalKeyStorage : public KeyStorage {
   public:
    struct Config {
      boost::filesystem::path sr25519_keypair_location{};
      boost::filesystem::path ed25519_keypair_location{};
      boost::filesystem::path p2p_keypair_location{};
      libp2p::crypto::Key::Type p2p_keypair_type =
          libp2p::crypto::Key::Type::Ed25519;
    };

    static outcome::result<std::shared_ptr<LocalKeyStorage>> create(
        const Config &c,
        std::shared_ptr<libp2p::crypto::CryptoProvider> crypto_provider,
        std::shared_ptr<libp2p::crypto::validator::KeyValidator> validator);

    ~LocalKeyStorage() override = default;

    crypto::SR25519Keypair getLocalSr25519Keypair() const override;
    crypto::ED25519Keypair getLocalEd25519Keypair() const override;
    libp2p::crypto::KeyPair getP2PKeypair() const override;

   private:
    LocalKeyStorage(
        std::shared_ptr<libp2p::crypto::CryptoProvider> crypto_provider,
        std::shared_ptr<libp2p::crypto::validator::KeyValidator> validator);

    /**
     * Loads a keypair from the provided file.
     * Supported file formats are PEM for Ed25519 and a TXT file with HEX
     * encoded keypair for Sr25519
     */
    outcome::result<libp2p::crypto::KeyPair> loadP2PKeypair(
        const boost::filesystem::path &file,
        libp2p::crypto::Key::Type type) const;

    outcome::result<crypto::ED25519Keypair> loadEd25519(
        const boost::filesystem::path &file) const;

    outcome::result<crypto::SR25519Keypair> loadSr25519(
        const boost::filesystem::path &file) const;

    /**
     * @warning it is not functioning yet, fails on public key validation
     */
    outcome::result<libp2p::crypto::KeyPair> loadRSA(
        const boost::filesystem::path &file) const;

    std::shared_ptr<libp2p::crypto::CryptoProvider> crypto_provider_;
    std::shared_ptr<libp2p::crypto::validator::KeyValidator> validator_;
    crypto::SR25519Keypair sr_25519_keypair_;
    crypto::ED25519Keypair ed_25519_keypair_;
    libp2p::crypto::KeyPair p2p_keypair_;
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_LOCAL_KEY_STORAGE_HPP
