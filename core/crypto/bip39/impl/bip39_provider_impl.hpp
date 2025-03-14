/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bip39/bip39_provider.hpp"

#include "crypto/bip39/dictionary.hpp"
#include "crypto/hasher.hpp"
#include "crypto/pbkdf2/pbkdf2_provider.hpp"
#include "crypto/random_generator.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {
  class Bip39ProviderImpl : public Bip39Provider {
   public:
    ~Bip39ProviderImpl() override = default;

    explicit Bip39ProviderImpl(std::shared_ptr<Pbkdf2Provider> pbkdf2_provider,
                               std::shared_ptr<CSPRNG> csprng,
                               std::shared_ptr<Hasher> hasher);

    std::string generatePhrase() const override;

    outcome::result<std::vector<uint8_t>> calculateEntropy(
        const std::vector<std::string> &word_list) const override;

    outcome::result<bip39::Bip39Seed> makeSeed(
        common::BufferView entropy, std::string_view password) const override;

    outcome::result<bip39::Bip39SeedAndJunctions> generateSeed(
        std::string_view mnemonic_phrase) const override;

   private:
    std::shared_ptr<Pbkdf2Provider> pbkdf2_provider_;
    std::shared_ptr<CSPRNG> csprng_;
    std::shared_ptr<Hasher> hasher_;
    bip39::Dictionary dictionary_;
    log::Logger logger_;
  };
}  // namespace kagome::crypto
