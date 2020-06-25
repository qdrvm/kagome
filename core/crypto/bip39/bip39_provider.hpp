/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIP39_PROVIDER_HPP
#define KAGOME_BIP39_PROVIDER_HPP

#include "common/buffer.hpp"
#include "crypto/bip39/bip39_types.hpp"

namespace kagome::crypto {

  /**
   * @class Bip39Provider allows creating seed from mnemonic wordlist
   */
  class Bip39Provider {
   public:
    virtual ~Bip39Provider() = default;

    /**
     * @brief calculates entropy from mnemonic
     * @param correct mnemonic word list
     * @return entropy value
     */
    virtual outcome::result<std::vector<uint8_t>> calculateEntropy(
        const std::vector<std::string> &word_list) = 0;

    /**
     * @brief makes seed from entropy
     * @param entropy entropy array
     * @return seed bytes
     */
    virtual outcome::result<bip39::Bip39Seed> makeSeed(
        gsl::span<const uint8_t> entropy, std::string_view password) = 0;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_BIP39_PROVIDER_HPP
