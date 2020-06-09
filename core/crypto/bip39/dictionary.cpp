/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/dictionary.hpp"

#include <gsl/gsl>
#include "crypto/bip39/entropy_accumulator.hpp"
#include "crypto/bip39/wordlist/english.hpp"

namespace kagome::crypto::bip39 {

  void Dictionary::initialize() {
    for (size_t i = 0; i < english::dictionary.size(); ++i) {
      auto token = EntropyToken(i);
      std::string_view word = gsl::at(english::dictionary, i);
      entropy_map_[word] = token;
    }
  }

  outcome::result<EntropyToken> Dictionary::findValue(std::string_view word) {
    auto loc = entropy_map_.find(word);
    if (entropy_map_.end() != loc) {
      return loc->second;
    }

    return DictionaryError::ENTRY_NOT_FOUND;
  }

}  // namespace kagome::crypto::bip39

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto::bip39, DictionaryError, error) {
  using E = kagome::crypto::bip39::DictionaryError;
  switch (error) {
    case E::ENTRY_NOT_FOUND:
      return "word not found";
  }
  return "unknown DictionaryError error";
}
