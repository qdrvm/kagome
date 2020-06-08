/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIP39_PROVIDER_HPP
#define KAGOME_BIP39_PROVIDER_HPP

#include "crypto/bip39/bip39_types.hpp"

namespace kagome::crypto {

  /**
   * @class Bip39Provider allows creating seed from mnemonic wordlist
   */
  class Bip39Provider {
   public:
    virtual ~Bip39Provider() = default;
    /**
     * @brief makes seed from mnemonic
     * @param phrase valid utf8 list of words from bip-39 word list
     * @return seed bytes
     */
    virtual outcome::result<bip39::Bip39Seed> makeSeed(
        std::string_view phrase) = 0;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_BIP39_PROVIDER_HPP
