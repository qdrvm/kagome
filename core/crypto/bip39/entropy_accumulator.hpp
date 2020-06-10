/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_BIP39_ENTROPY_HPP
#define KAGOME_CRYPTO_BIP39_ENTROPY_HPP

#include <boost/assert.hpp>
#include "outcome/outcome.hpp"

#include <bitset>
#include <vector>

namespace kagome::crypto::bip39 {

  enum class Bip39EntropyError {
    WRONG_WORDS_COUNT = 1,
    STORAGE_NOT_COMPLETE,
    STORAGE_IS_FULL,
  };

  struct EntropyToken : public std::bitset<11> {
    using Parent = std::bitset<11>;
    using Parent::bitset;
  };

  /**
   * @class EntropyAccumulator accumulates and provides entropy and checksum
   */
  class EntropyAccumulator {
   public:
    /**
     * @brief create class instance
     * @param words_count number of words in mnemonic phrase
     */
    static outcome::result<EntropyAccumulator> create(size_t words_count);

    /**
     * @brief append a new entropy token
     * @param value token
     * @return success or error if storage is full
     */
    outcome::result<void> append(const EntropyToken &value);

    /**
     * @return entropy as byte array
     */
    outcome::result<std::vector<uint8_t>> getEntropy() const;

    /**
     * @brief checksum is a part of last byte
     * @return checksum
     */
    outcome::result<uint8_t> getChecksum() const;

    /**
     * @brief calculates checksum of significant bits
     * @return checksum value
     */
    outcome::result<uint8_t> calculateChecksum() const;

   private:
    /**
     * @param bits_count total bits count (depends on words count)
     * @param checksum_bits_count number of bits in checksum byte
     */
    EntropyAccumulator(size_t bits_count, size_t checksum_bits_count);

    std::vector<uint8_t> bits_;
    const size_t total_bits_count_;
    const size_t checksum_bits_count_;
  };
}  // namespace kagome::crypto::bip39

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto::bip39, Bip39EntropyError);

#endif  // KAGOME_CRYPTO_BIP39_ENTROPY_HPP
