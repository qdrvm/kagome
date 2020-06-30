/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_BIP39_DICTIONARIES_HPP
#define KAGOME_CRYPTO_BIP39_DICTIONARIES_HPP

#include "crypto/bip39/entropy_accumulator.hpp"
#include "outcome/outcome.hpp"

#include <unordered_map>

namespace kagome::crypto::bip39 {

  enum class DictionaryError {
    ENTRY_NOT_FOUND = 1,
  };

  /**
   * @class Dictionary keeps and provides correspondence between mnemonic words
   * and entropy value. Only english dictionary is supported for now
   */
  class Dictionary {
   public:
    /**
     * @brief initializes dictionary
     */
    void initialize();

    /**
     * @brief looks for word in dictionary
     * @param word word to look for
     * @return entropy value or error if not found
     */
    outcome::result<EntropyToken> findValue(std::string_view word);

   private:
    std::unordered_map<std::string_view, EntropyToken> entropy_map_;
  };
}  // namespace kagome::crypto::bip39

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto::bip39, DictionaryError);

#endif  // KAGOME_CRYPTO_BIP39_DICTIONARIES_HPP
