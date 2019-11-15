/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/local_key_storage.hpp"

#include "application/impl/crypto_key_reader/crypto_key_reader_util.hpp"

namespace kagome::application {

  outcome::result<std::shared_ptr<LocalKeyStorage>> LocalKeyStorage::create(
      const Config &c,
      std::shared_ptr<libp2p::crypto::KeyGenerator> generator,
      std::shared_ptr<libp2p::crypto::validator::KeyValidator> validator) {
    auto storage = LocalKeyStorage(std::move(generator), std::move(validator));
    OUTCOME_TRY(
        p2p_pair,
        storage.loadP2PKeypair(c.p2p_keypair_location, c.p2p_keypair_type));
    storage.p2p_keypair_ = p2p_pair;
    OUTCOME_TRY(ed_pair, storage.loadEd25519(c.ed25519_keypair_location));
    storage.ed_25519_keypair_ = ed_pair;
    OUTCOME_TRY(sr_pair, storage.loadSr25519(c.sr25519_keypair_location));
    storage.sr_25519_keypair_ = sr_pair;
    return std::make_shared<LocalKeyStorage>(std::move(storage));
  }

  LocalKeyStorage::LocalKeyStorage(
      std::shared_ptr<libp2p::crypto::KeyGenerator> generator,
      std::shared_ptr<libp2p::crypto::validator::KeyValidator> validator)
      : generator_{std::move(generator)}, validator_{std::move(validator)} {
    BOOST_ASSERT(generator_ != nullptr);
    BOOST_ASSERT(validator_ != nullptr);
  }

  kagome::crypto::SR25519Keypair LocalKeyStorage::getLocalSr25519Keypair()
      const {
    return sr_25519_keypair_;
  }

  kagome::crypto::ED25519Keypair LocalKeyStorage::getLocalEd25519Keypair()
      const {
    return ed_25519_keypair_;
  }

  libp2p::crypto::KeyPair LocalKeyStorage::getP2PKeypair() const {
    return p2p_keypair_;
  }

  outcome::result<libp2p::crypto::KeyPair> LocalKeyStorage::loadP2PKeypair(
      const boost::filesystem::path &file,
      libp2p::crypto::Key::Type type) const {
    OUTCOME_TRY(bytes, readPrivKeyFromPEM(file, type));
    libp2p::crypto::PrivateKey private_key;
    private_key.type = type;
    private_key.data.resize(bytes.size());
    std::copy(bytes.begin(), bytes.end(), private_key.data.begin());

    OUTCOME_TRY(public_key, generator_->derivePublicKey(private_key));
    OUTCOME_TRY(validator_->validate(public_key));

    return libp2p::crypto::KeyPair{std::move(public_key),
                                   std::move(private_key)};
  }

  outcome::result<crypto::ED25519Keypair> LocalKeyStorage::loadEd25519(
      const boost::filesystem::path &file) const {
    OUTCOME_TRY(pair, loadP2PKeypair(file, libp2p::crypto::Key::Type::Ed25519));
    auto &&[public_key, private_key] = pair;
    crypto::ED25519Keypair keypair;
    std::copy(public_key.data.begin(),
              public_key.data.end(),
              keypair.public_key.begin());
    std::copy(private_key.data.begin(),
              private_key.data.end(),
              keypair.private_key.begin());
    return keypair;
  }

  outcome::result<crypto::SR25519Keypair> LocalKeyStorage::loadSr25519(
      const boost::filesystem::path &file) const {
    using namespace crypto::constants::sr25519;
    OUTCOME_TRY(bytes, readKeypairFromHexFile(file));
    crypto::SR25519Keypair keypair;
    BOOST_ASSERT(KEYPAIR_SIZE == bytes.size());
    std::copy_n(bytes.begin(), PUBLIC_SIZE, keypair.public_key.begin());
    std::copy_n(
        bytes.begin() + PUBLIC_SIZE, SECRET_SIZE, keypair.secret_key.begin());
    return keypair;
  }

}  // namespace kagome::application
