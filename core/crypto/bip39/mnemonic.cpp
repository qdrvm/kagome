/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/mnemonic.hpp"

#include <codecvt>
#include <locale>

#include <boost/algorithm/string.hpp>
#include "common/logger.hpp"
#include "crypto/bip39/bip39_types.hpp"

namespace kagome::crypto::bip39 {

  namespace {
    constexpr auto kMnemonicLoggerString = "Mnemonic";
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

  outcome::result<Mnemonic> Mnemonic::parse(std::string_view phrase) {
    if (phrase.empty() || !isValidUtf8(std::string(phrase))) {
      return bip39::MnemonicError::INVALID_MNEMONIC;
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

    if (mnemonic_list.find("/") != std::string_view::npos) {
      common::createLogger(kMnemonicLoggerString)
          ->error("junctions are not supported yet");
      return bip39::MnemonicError::INVALID_MNEMONIC;
    }

    // split word list into separate words
    std::vector<std::string> word_list;
    boost::split(word_list, mnemonic_list, boost::algorithm::is_space());

    Mnemonic mnemonic{std::move(word_list), std::string(password)};
    return mnemonic;
    //
  }
}  // namespace kagome::crypto::bip39

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto::bip39, MnemonicError, e) {
  using Error = kagome::crypto::bip39::MnemonicError;
  switch (e) {
    case Error::INVALID_MNEMONIC:
      return "Mnemonic provided is not valid";
  }
  return "unknown MnemonicError";
}
