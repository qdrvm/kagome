/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/entropy_accumulator.hpp"

#include "crypto/sha/sha256.hpp"

namespace kagome::crypto::bip39 {
  outcome::result<EntropyAccumulator> EntropyAccumulator::create(
      size_t words_count) {
    switch (words_count) {
      case 12:
        return EntropyAccumulator(132, 4);
      case 15:
        return EntropyAccumulator(165, 5);
      case 18:
        return EntropyAccumulator(198, 6);
      case 21:
        return EntropyAccumulator(231, 7);
      case 24:
        return EntropyAccumulator(264, 8);
      default:
        break;
    }
    return Bip39EntropyError::WRONG_WORDS_COUNT;
  }

  outcome::result<std::vector<uint8_t>> EntropyAccumulator::getEntropy() const {
    if (bits_.size() != total_bits_count_) {
      return Bip39EntropyError::STORAGE_NOT_COMPLETE;
    }

    // convert data
    size_t bytes_count = (total_bits_count_ - checksum_bits_count_) / 8;
    std::vector<uint8_t> res;
    res.reserve(bytes_count);
    auto it = bits_.begin();
    for (size_t i = 0; i < bytes_count; ++i) {
      uint8_t byte = 0;
      for (size_t j = 0; j < 8u; ++j) {
        byte <<= 1u;
        byte += *it++;
      }

      res.push_back(byte);
    }

    return res;
  }

  outcome::result<uint8_t> EntropyAccumulator::getChecksum() const {
    if (bits_.size() != total_bits_count_) {
      return Bip39EntropyError::STORAGE_NOT_COMPLETE;
    }

    uint8_t checksum = 0u;
    auto it = bits_.rbegin();
    for (auto i = 0u; i < checksum_bits_count_; ++i) {
      checksum += (*it++ << i);
    }

    return checksum;
  }

  outcome::result<void> EntropyAccumulator::append(const EntropyToken &value) {
    if (bits_.size() + value.size() > total_bits_count_) {
      return Bip39EntropyError::STORAGE_IS_FULL;
    }

    for (size_t i = 0; i < value.size(); ++i) {
      // bits order is little-endian, but we need big endian, reverse it
      auto position = value.size() - i - 1;
      uint8_t v = value.test(position) ? 1 : 0;
      bits_.push_back(v);
    }

    return outcome::success();
  }

  outcome::result<uint8_t> EntropyAccumulator::calculateChecksum() const {
    OUTCOME_TRY(entropy, getEntropy());
    auto hash = sha256(entropy);
    return hash[0] >> static_cast<uint8_t>(8 - checksum_bits_count_);
  }

  EntropyAccumulator::EntropyAccumulator(size_t bits_count,
                                         size_t checksum_bits_count)
      : total_bits_count_{bits_count},
        checksum_bits_count_{checksum_bits_count} {
    BOOST_ASSERT_MSG((bits_count - checksum_bits_count) % 32 == 0,
                     "invalid bits count");
    BOOST_ASSERT_MSG(bits_count <= 264 && bits_count >= 132,
                     "unsupported bits count");

    bits_.reserve(bits_count);
  }

}  // namespace kagome::crypto::bip39

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto::bip39, Bip39EntropyError, error) {
  using E = kagome::crypto::bip39::Bip39EntropyError;
  switch (error) {
    case E::WRONG_WORDS_COUNT:
      return "invalid or unsupported words count";
    case E::STORAGE_NOT_COMPLETE:
      return "cannot get info from storage while it is still not complete";
    case E::STORAGE_IS_FULL:
      return "cannot put more data into storage, it is full";
  }

  return "unknown Bip39EntropyError error";
}
