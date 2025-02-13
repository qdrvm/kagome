/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/impl/bip39_provider_impl.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include "crypto/bip39/entropy_accumulator.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/bip39/wordlist/english.hpp"
#include "crypto/sha/sha256.hpp"

namespace kagome::crypto {

  static const std::vector<std::string> kDevWords{
      "bottom",
      "drive",
      "obey",
      "lake",
      "curtain",
      "smoke",
      "basket",
      "hold",
      "race",
      "lonely",
      "fit",
      "walk",
  };

  Bip39ProviderImpl::Bip39ProviderImpl(
      std::shared_ptr<Pbkdf2Provider> pbkdf2_provider,
      std::shared_ptr<CSPRNG> csprng,
      std::shared_ptr<Hasher> hasher)
      : pbkdf2_provider_{std::move(pbkdf2_provider)},
        csprng_{std::move(csprng)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("Bip39Provider", "bip39")} {
    BOOST_ASSERT(pbkdf2_provider_ != nullptr);
    BOOST_ASSERT(csprng_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
    dictionary_.initialize();
  }

  // https://github.com/rust-bitcoin/rust-bip39/blob/b100bf3e22891498cb6e0b1c53fd629dab7b34de/src/lib.rs#L267
  // https://github.com/rust-bitcoin/rust-bip39/blob/b100bf3e22891498cb6e0b1c53fd629dab7b34de/src/lib.rs#L204
  std::string Bip39ProviderImpl::generatePhrase() const {
    size_t n_words = 12;
    size_t n_check_bits = 4;
    auto n_bytes = (n_words / 3) * 4;
    SecureBuffer<> bytes(n_bytes);
    csprng_->fillRandomly(bytes);
    auto check = sha256(bytes);
    std::vector<bool> bits((n_bytes * 8) + n_check_bits);
    for (size_t i = 0; i < n_bytes; ++i) {
      for (size_t j = 0; j < 8; ++j) {
        bits[(i * 8) + j] = (bytes[i] & (1 << (7 - j))) > 0;
      }
    }
    for (size_t i = 0; i < n_bytes / 4; ++i) {
      bits[(8 * n_bytes) + i] = (check.at(i / 8) & (1 << (7 - (i % 8)))) > 0;
    }
    std::vector<std::string> words;
    for (size_t i = 0; i < n_words; ++i) {
      size_t idx = 0;
      for (size_t j = 0; j < 11; ++j) {
        if (bits[(i * 11) + j]) {
          idx += 1 << (10 - j);
        }
      }
      words.emplace_back(bip39::english::dictionary.at(idx));
    }
    return fmt::format("{}", fmt::join(words, " "));
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
      common::BufferView entropy, std::string_view password) const {
    constexpr size_t iterations_count = 2048u;
    constexpr auto default_salt = "mnemonic";

    common::Buffer salt{};
    salt.put(default_salt);
    salt.put(password);

    OUTCOME_TRY(
        key,
        pbkdf2_provider_->deriveKey(entropy,
                                    salt,
                                    iterations_count,
                                    bip39::constants::BIP39_SEED_LEN_512));
    return bip39::Bip39Seed::from(std::move(key));
  }

  outcome::result<bip39::Bip39SeedAndJunctions> Bip39ProviderImpl::generateSeed(
      std::string_view mnemonic_phrase) const {
    OUTCOME_TRY(mnemonic, bip39::Mnemonic::parse(mnemonic_phrase));
    auto seed_res = [&]() -> outcome::result<bip39::Bip39Seed> {
      if (auto words = mnemonic.words()) {
        if (words->empty()) {
          words = &kDevWords;
        }
        OUTCOME_TRY(entropy, calculateEntropy(*words));
        return makeSeed(entropy, mnemonic.password);
      }
      return std::get<bip39::Bip39Seed>(std::move(mnemonic.seed));
    }();
    OUTCOME_TRY(seed, seed_res);
    bip39::Bip39SeedAndJunctions result{
        .seed = std::move(seed),
        .junctions = {},
    };

    for (auto &junction : mnemonic.junctions) {
      result.junctions.emplace_back(junction.raw(*hasher_));
    }
    return result;
  }
}  // namespace kagome::crypto
