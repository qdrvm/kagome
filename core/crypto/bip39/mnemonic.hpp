/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>

#include "crypto/bip39/bip39_types.hpp"

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::crypto::bip39 {
  enum class MnemonicError {
    INVALID_MNEMONIC = 1,
  };

  struct Junction {
    bool hard;
    boost::variant<uint64_t, std::string> index;

    RawJunction raw(const crypto::Hasher &hasher) const;
  };

  struct Mnemonic {
    using Words = std::vector<std::string>;

    std::variant<Bip39Seed, Words> seed;
    std::string password;
    std::vector<Junction> junctions;

    const Words *words() const {
      return std::get_if<Words>(&seed);
    }

    /**
     * @brief parse mnemonic from phrase
     * @param phrase valid utf8 list of words from bip-39 word list
     * @return Mnemonic instance
     */
    static outcome::result<Mnemonic> parse(std::string_view phrase);
  };
}  // namespace kagome::crypto::bip39

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto::bip39, MnemonicError);
