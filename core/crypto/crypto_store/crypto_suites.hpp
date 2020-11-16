/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SUITES_HPP
#define KAGOME_CRYPTO_SUITES_HPP

#include "crypto/ed25519_provider.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

  /**
   * Combination of several crypto primitives belonging to one algorithm
   */
  template <typename PublicKeyT,
            typename PrivateKeyT,
            typename KeypairT,
            typename SeedT>
  struct CryptoSuite {
    using PublicKey = PublicKeyT;
    using PrivateKey = PrivateKeyT;
    using Keypair = KeypairT;
    using Seed = SeedT;

    virtual ~CryptoSuite() = default;

    /**
     * Genereate a keypair from \param seed (mind that in some algorithms a seed
     * is a private key)
     */
    virtual Keypair generateKeypair(const Seed &seed) const noexcept = 0;
    /**
     * Generate a random keypair (randomness source is determined by an
     * underlying crypto provider)
     */
    virtual Keypair generateRandomKeypair() const noexcept = 0;

    /**
     * Create a keypair from a public key and a private key
     * @note Although it is typically just a structure with two fields, from the
     * compiler point of view they all are different types, thus this
     * convenience method emerges
     */
    virtual Keypair composeKeypair(PublicKey pub,
                                   PrivateKey priv) const noexcept = 0;
    /**
     * Extrace the private key and the public key from a keypair
     * @see composeKeypair()
     */
    virtual std::pair<PublicKey, PrivateKey> decomposeKeypair(
        const Keypair &kp) const noexcept = 0;

    /**
     * Create a public key from its bytes
     */
    virtual outcome::result<PublicKey> toPublicKey(
        gsl::span<const uint8_t> bytes) const noexcept = 0;

    /**
     * Create a seed from its bytes
     */
    virtual outcome::result<Seed> toSeed(
        gsl::span<const uint8_t> bytes) const noexcept = 0;
  };

  class Ed25519Suite : public CryptoSuite<Ed25519PublicKey,
                                          Ed25519PrivateKey,
                                          Ed25519Keypair,
                                          Ed25519Seed> {
   public:
    explicit Ed25519Suite(std::shared_ptr<Ed25519Provider> ed_provider)
        : ed_provider_{std::move(ed_provider)} {
      BOOST_ASSERT(ed_provider_ != nullptr);
    }

    ~Ed25519Suite() override = default;

    Ed25519Keypair generateRandomKeypair() const noexcept override {
      return ed_provider_->generateKeypair();
    }

    Ed25519Keypair generateKeypair(
        const Ed25519Seed &seed) const noexcept override {
      return ed_provider_->generateKeypair(seed);
    }

    Ed25519Keypair composeKeypair(PublicKey pub,
                                  PrivateKey priv) const noexcept override {
      return Ed25519Keypair{.secret_key = std::move(priv),
                            .public_key = std::move(pub)};
    }

    std::pair<PublicKey, PrivateKey> decomposeKeypair(
        const Ed25519Keypair &kp) const noexcept override {
      return {kp.public_key, kp.secret_key};
    }

    outcome::result<PublicKey> toPublicKey(
        gsl::span<const uint8_t> bytes) const noexcept override {
      return Ed25519PublicKey::fromSpan(bytes);
    }

    outcome::result<Seed> toSeed(
        gsl::span<const uint8_t> bytes) const noexcept override {
      return Ed25519Seed::fromSpan(bytes);
    }

   private:
    std::shared_ptr<Ed25519Provider> ed_provider_;
  };

  class Sr25519Suite : public CryptoSuite<Sr25519PublicKey,
                                          Sr25519SecretKey,
                                          Sr25519Keypair,
                                          Sr25519Seed> {
   public:
    explicit Sr25519Suite(std::shared_ptr<Sr25519Provider> sr_provider)
        : sr_provider_{std::move(sr_provider)} {
      BOOST_ASSERT(sr_provider_ != nullptr);
    }

    ~Sr25519Suite() override = default;

    Sr25519Keypair generateRandomKeypair() const noexcept override {
      return sr_provider_->generateKeypair();
    }

    Sr25519Keypair generateKeypair(
        const Sr25519Seed &seed) const noexcept override {
      return sr_provider_->generateKeypair(seed);
    }

    Sr25519Keypair composeKeypair(PublicKey pub,
                                  PrivateKey priv) const noexcept override {
      return Sr25519Keypair{.secret_key = std::move(priv),
                            .public_key = std::move(pub)};
    }

    std::pair<PublicKey, PrivateKey> decomposeKeypair(
        const Sr25519Keypair &kp) const noexcept override {
      return {kp.public_key, kp.secret_key};
    }

    outcome::result<PublicKey> toPublicKey(
        gsl::span<const uint8_t> bytes) const noexcept override {
      return Sr25519PublicKey::fromSpan(bytes);
    }

    outcome::result<Seed> toSeed(
        gsl::span<const uint8_t> bytes) const noexcept override {
      return Sr25519Seed::fromSpan(bytes);
    }

   private:
    std::shared_ptr<Sr25519Provider> sr_provider_;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SUITES_HPP
