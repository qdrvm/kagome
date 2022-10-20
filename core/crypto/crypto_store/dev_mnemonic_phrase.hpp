/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_DEV_MNEMONIC_PHRASE_HPP
#define KAGOME_CRYPTO_DEV_MNEMONIC_PHRASE_HPP

#include <map>
#include <optional>

#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {
  class DevMnemonicPhrase {
    static constexpr std::string_view kWords =
        "bottom drive obey lake curtain smoke basket hold race lonely fit walk";

    // junctions are not supported yet
    std::map<std::string, std::pair<Ed25519Seed, Sr25519Seed>, std::less<>>
        precomputed;

    DevMnemonicPhrase() {
      alice = precompute(
          "//Alice",
          "abf8e5bdbe30c65656c0a3cbd181ff8a56294a69dfedd27982aace4a76909115",
          "e5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a");
      bob = precompute(
          "//Bob",
          "3b7b60af2abcd57ba401ab398f84f4ca54bd6b2140d2503fbcf3286535fe3ff1",
          "398f0c28f98885e046333d4a41c19cee4c37368a9832c6502f6cfd182e2aef89");
      charlie = precompute(
          "//Charlie",
          "072c02fa1409dc37e03a4ed01703d4a9e6bba9c228a49a00366e9630a97cba7c",
          "bc1ede780f784bb6991a585e4f6e61522c14e1cae6ad0895fb57b9a205a8f938");
      dave = precompute(
          "//Dave",
          "771f47d3caf8a2ee40b0719e1c1ecbc01d73ada220cf08df12a00453ab703738",
          "868020ae0687dda7d57565093a69090211449845a7e11453612800b663307246");
      eve = precompute(
          "//Eve",
          "bef5a3cd63dd36ab9792364536140e5a0cce6925969940c431934de056398556",
          "786ad0e2df456fe43dd1f91ebca22e235bc162e0bb8d53c633e8c85b2af68b7a");
      ferdie = precompute(
          "//Ferdie",
          "1441e38eb309b66e9286867a5cd05902b05413eb9723a685d4d77753d73d0a1d",
          "42438b7883391c05512a938e36c2df0131e088b3756d6aa7a755fbff19d2f842");
    }

    std::string precompute(std::string_view junctions,
                           std::string_view ed25519_seed,
                           std ::string_view sr25519_seed) {
      std::string mnemonic_phrase{kWords};
      mnemonic_phrase += junctions;
      precomputed[mnemonic_phrase] = {
          Ed25519Seed::fromHex(ed25519_seed).value(),
          Sr25519Seed::fromHex(sr25519_seed).value(),
      };
      return mnemonic_phrase;
    }

   public:
    static const DevMnemonicPhrase &get() {
      static const DevMnemonicPhrase self;
      return self;
    }

    template <typename Seed>
    std::optional<Seed> find(std::string_view mnemonic_phrase) const {
      if constexpr (std::is_same_v<
                        Seed,
                        Ed25519Seed> || std::is_same_v<Seed, Sr25519Seed>) {
        auto it = precomputed.find(mnemonic_phrase);
        if (it != precomputed.end()) {
          if constexpr (std::is_same_v<Seed, Ed25519Seed>) {
            return it->second.first;
          }
          if constexpr (std::is_same_v<Seed, Sr25519Seed>) {
            return it->second.second;
          }
        }
      }
      return std::nullopt;
    }

    std::string alice;
    std::string bob;
    std::string charlie;
    std::string dave;
    std::string eve;
    std::string ferdie;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_DEV_MNEMONIC_PHRASE_HPP
