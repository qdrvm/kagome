/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIP39_PROVIDER_IMPL_HPP
#define KAGOME_BIP39_PROVIDER_IMPL_HPP

#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/bip39/dictionary.hpp"

#include "common/logger.hpp"
#include "crypto/pbkdf2/pbkdf2_provider.hpp"

namespace kagome::crypto {
  class Bip39ProviderImpl : public Bip39Provider {
   public:
    ~Bip39ProviderImpl() override = default;

    explicit Bip39ProviderImpl(std::shared_ptr<Pbkdf2Provider> pbkdf2_provider);

    outcome::result<std::vector<uint8_t>> calculateEntropy(
        const std::vector<std::string> &word_list) override;

    outcome::result<bip39::Bip39Seed> makeSeed(
        gsl::span<const uint8_t> entropy, std::string_view password) override;

   private:
    std::shared_ptr<Pbkdf2Provider> pbkdf2_provider_;
    bip39::Dictionary dictionary_;
    common::Logger logger_;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_BIP39_PROVIDER_IMPL_HPP
