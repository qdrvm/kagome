/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/impl/bip39_provider_impl.hpp"

#include "crypto/bip39/entropy_accumulator.hpp"
#include "crypto/bip39/mnemonic.hpp"

namespace kagome::crypto {

  Bip39ProviderImpl::Bip39ProviderImpl(
      std::shared_ptr<Pbkdf2Provider> pbkdf2_provider)
      : pbkdf2_provider_(std::move(pbkdf2_provider)),
        logger_{log::createLogger("Bip39Provider", "bip39")} {
    dictionary_.initialize();
  }

  outcome::result<std::vector<uint8_t>> Bip39ProviderImpl::calculateEntropy(
      const std::vector<std::string> &word_list) const {
    // make entropy accumulator
    OUTCOME_TRY(entropy_accumulator,
                bip39::EntropyAccumulator::create(word_list.size()));

    // accumulate entropy
    for (const auto &w : word_list) {
      OUTCOME_TRY(entropy_token, dictionary_.findValue(w));
      OUTCOME_TRY(entropy_accumulator.append(entropy_token));
    }

    OUTCOME_TRY(mnemonic_checksum, entropy_accumulator.getChecksum());
    OUTCOME_TRY(calculated_checksum, entropy_accumulator.calculateChecksum());

    if (mnemonic_checksum != calculated_checksum) {
      return bip39::MnemonicError::INVALID_MNEMONIC;
    }

    // finally get entropy
    return entropy_accumulator.getEntropy();
  }

  outcome::result<bip39::Bip39Seed> Bip39ProviderImpl::makeSeed(
      gsl::span<const uint8_t> entropy, std::string_view password) const {
    constexpr size_t iterations_count = 2048u;
    constexpr auto default_salt = "mnemonic";

    common::Buffer salt{};
    salt.put(default_salt);
    salt.put(password);

    return pbkdf2_provider_->deriveKey(
        entropy, salt, iterations_count, bip39::constants::BIP39_SEED_LEN_512);
  }

  outcome::result<bip39::Bip39Seed> Bip39ProviderImpl::generateSeed(
      std::string_view mnemonic_phrase) const {
    OUTCOME_TRY(mnemonic, bip39::Mnemonic::parse(mnemonic_phrase));
    OUTCOME_TRY(entropy, calculateEntropy(mnemonic.words));
    return makeSeed(entropy, mnemonic.password);
  }
}  // namespace kagome::crypto
