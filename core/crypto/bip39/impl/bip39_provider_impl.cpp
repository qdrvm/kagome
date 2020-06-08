/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/impl/bip39_provider_impl.hpp"

#include "crypto/bip39/bip39_entropy.hpp"
#include "crypto/bip39/mnemonic.hpp"

namespace kagome::crypto {
  Bip39ProviderImpl::Bip39ProviderImpl(
      std::shared_ptr<Pbkdf2Provider> pbkdf2_provider)
      : pbkdf2_provider_(std::move(pbkdf2_provider)),
        dictionary_{},
        logger_{common::createLogger("Bip39Provider")} {
    dictionary_.initialize();
  }

  outcome::result<bip39::Bip39Seed> Bip39ProviderImpl::makeSeed(
      std::string_view phrase) {
    constexpr size_t iterations_count = 2048u;
    constexpr auto default_salt = "mnemonic";

    OUTCOME_TRY(mnemonic, bip39::Mnemonic::parse(phrase));
    common::Buffer salt{};
    salt.put(default_salt);
    salt.put(mnemonic.password);

    // make entropy accumulator
    OUTCOME_TRY(entropy_accumulator,
                bip39::Bip39Entropy::create(mnemonic.words.size()));

    // accummulate entropy
    for (auto &&w : mnemonic.words) {
      OUTCOME_TRY(entropy_token, dictionary_.findValue(w));
      OUTCOME_TRY(entropy_accumulator.append(entropy_token));
    }

    if (entropy_accumulator.getChecksum()
        != entropy_accumulator.calculateChecksum()) {
      return bip39::MnemonicError::INVALID_MNEMONIC;
    }

    // finally get entropy
    OUTCOME_TRY(entropy, entropy_accumulator.getEntropy());

    OUTCOME_TRY(
        buffer,
        pbkdf2_provider_->deriveKey(entropy,
                                    salt,
                                    iterations_count,
                                    bip39::constants::BIP39_SEED_LEN_512));

    return bip39::Bip39Seed::fromSpan(buffer.toVector());
  }
}  // namespace kagome::crypto
