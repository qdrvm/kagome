/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/impl/bip39_provider_impl.hpp"

#include <codecvt>
#include <locale>

#include <boost/algorithm/string.hpp>
#include "crypto/bip39/impl/detail/bip39_entropy.hpp"

namespace kagome::crypto {
  Bip39ProviderImpl::Bip39ProviderImpl(
      std::shared_ptr<Pbkdf2Provider> pbkdf2_provider)
      : pbkdf2_provider_(std::move(pbkdf2_provider)),
        dictionary_{},
        logger_{common::createLogger("Bip39Provider")} {
    dictionary_.initialize();
  }

  namespace {
    /**
     * @return true if string s is a valid utf-8, false otherwise
     */
    bool isValidUtf8(const std::string &s) {
      // cannot replace for std::string_view for security reasons
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
          utf16conv;
      try {
        auto utf16 = utf16conv.from_bytes(s.data());
      } catch (...) {
        return false;
      }
      return true;
    }
  }  // namespace

  outcome::result<bip39::Bip39Seed> Bip39ProviderImpl::makeSeed(
      std::string_view phrase) {
    const size_t iterations_count = 2048;
    std::vector<uint8_t> salt = {'m', 'n', 'e', 'm', 'o', 'n', 'i', 'c'};

    if (phrase.empty() || !isValidUtf8(std::string(phrase))) {
      return bip39::Bip39ProviderError::INVALID_MNEMONIC;
    }

    if (phrase.find("/") != std::string_view::npos or phrase.find("//")) {
      logger_->error("junctions are not supported yet");
      return bip39::Bip39ProviderError::INVALID_MNEMONIC;
    }

    auto password_pos = phrase.find("///");
    std::string_view mnemonic_list;
    std::string_view password;
    if (password_pos != std::string_view::npos) {
      // need to normalize password utf8 nfkd
      password = phrase.substr(password_pos + 3);
      mnemonic_list = phrase.substr(0, password_pos);
    } else {
      mnemonic_list = phrase;
    }

    // split word list into separate words
    std::vector<std::string> word_list;
    boost::split(word_list, mnemonic_list, boost::algorithm::is_space());

    // make entropy accumulator
    OUTCOME_TRY(entropy_accumulator,
                bip39::Bip39Entropy::create(word_list.size()));

    // accummulate entropy
    for (auto &&w : word_list) {
      OUTCOME_TRY(entropy_token, dictionary_.findValue(w));
      OUTCOME_TRY(entropy_accumulator.append(entropy_token));
    }

    if (entropy_accumulator.getChecksum()
        != entropy_accumulator.calculateChecksum()) {
      return bip39::Bip39ProviderError::INVALID_MNEMONIC;
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

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto::bip39, Bip39ProviderError, e) {
  using Error = kagome::crypto::bip39::Bip39ProviderError;
  switch (e) {
    case Error::INVALID_MNEMONIC:
      return "Mnemonic provided is not valid";
  }
  return "unknown Bip39ProviderError";
}
