/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_BIP39_MNEMONIC_HPP
#define KAGOME_CRYPTO_BIP39_MNEMONIC_HPP

#include <string>
#include <vector>

#include "outcome/outcome.hpp"

namespace kagome::crypto::bip39 {
  struct Mnemonic {
    std::vector<std::string> words;
    std::string password;
    /**
     * @brief parse mnemonic from phrase
     * @param phrase mnemonic phrase
     * @return Mnemonic instance
     */
    static outcome::result<Mnemonic> parse(std::string_view phrase);
  };
}  // namespace kagome::crypto::bip39

#endif  // KAGOME_CRYPTO_BIP39_MNEMONIC_HPP
