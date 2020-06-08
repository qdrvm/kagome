/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIP39_TYPES_HPP
#define KAGOME_BIP39_TYPES_HPP

#include "common/blob.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto::bip39 {
  namespace constants {
    constexpr size_t BIP39_SEED_LEN_512 = 64u;
    constexpr size_t BIP39_WORDLIST_LEN = 2048u;

    /** Valid entropy lengths */
    constexpr size_t BIP39_ENTROPY_LEN_128 = 16u;
    constexpr size_t BIP39_ENTROPY_LEN_160 = 20u;
    constexpr size_t BIP39_ENTROPY_LEN_192 = 24u;
    constexpr size_t BIP39_ENTROPY_LEN_224 = 28u;
    constexpr size_t BIP39_ENTROPY_LEN_256 = 32u;
    constexpr size_t BIP39_ENTROPY_LEN_288 = 36u;
    constexpr size_t BIP39_ENTROPY_LEN_320 = 40u;

    std::array<size_t, 7> valid_entropy_lengths = {BIP39_ENTROPY_LEN_128,
                                                   BIP39_ENTROPY_LEN_160,
                                                   BIP39_ENTROPY_LEN_192,
                                                   BIP39_ENTROPY_LEN_224,
                                                   BIP39_ENTROPY_LEN_256,
                                                   BIP39_ENTROPY_LEN_288,
                                                   BIP39_ENTROPY_LEN_320};
  }  // namespace constants

  using Bip39Seed = common::Blob<constants::BIP39_SEED_LEN_512>;

  enum class Bip39ProviderError {
    INVALID_MNEMONIC = 1,
  };
}  // namespace kagome::crypto::bip39

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto::bip39, Bip39ProviderError);

#endif  // KAGOME_BIP39_TYPES_HPP
