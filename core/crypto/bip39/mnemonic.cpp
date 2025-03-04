/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/mnemonic.hpp"

#include <google/protobuf/stubs/port.h>

#include <charconv>
#include <codecvt>
#include <locale>

#include <boost/algorithm/string.hpp>

#include "common/visitor.hpp"
#include "crypto/bip39/bip39_types.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto::bip39 {

  namespace {
    /**
     * @brief Checks if a given string is a valid UTF-8 encoded sequence.
     *
     * @param s The input string in UTF-8 encoding.
     * @return true if the string is valid UTF-8, otherwise false.
     *
     * @details
     * The function performs a byte-by-byte validation of the input string by
     * analyzing the leading bits of each byte:
     * - 0xxxxxxx : Single-byte ASCII (valid)
     * - 110xxxxx : Start of a 2-byte sequence
     *              (must be followed by 10xxxxxx)
     * - 1110xxxx : Start of a 3-byte sequence
     *              (must be followed by two 10xxxxxx bytes)
     * - 11110xxx : Start of a 4-byte sequence
     *              (must be followed by three 10xxxxxx bytes)
     *
     * Additional checks:
     * - Surrogate pair range (U+D800 to U+DFFF) is invalid in UTF-8.
     * - Codepoints above U+10FFFF are not allowed.
     */
    bool isValidUtf8(std::string_view s) {
      size_t i = 0, len = s.size();
      while (i < len) {
        auto c = static_cast<uint8_t>(s[i]);
        if (c <= 0x7F) {
          // 1-byte character (ASCII)
          i++;
        } else if ((c & 0xE0) == 0xC0) {
          // 2-byte character (110xxxxx 10xxxxxx)
          if (i + 1 >= len || (s[i + 1] & 0xC0) != 0x80) {
            return false;
          }
          i += 2;
        } else if ((c & 0xF0) == 0xE0) {
          // 3-byte character (1110xxxx 10xxxxxx 10xxxxxx)
          if (i + 2 >= len || (s[i + 1] & 0xC0) != 0x80
              || (s[i + 2] & 0xC0) != 0x80) {
            return false;
          }
          uint32_t codepoint =
              ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
          if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
            return false;  // Invalid UTF-16 surrogate pair range
          }
          i += 3;
        } else if ((c & 0xF8) == 0xF0) {
          // 4-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
          if (i + 3 >= len || (s[i + 1] & 0xC0) != 0x80
              || (s[i + 2] & 0xC0) != 0x80 || (s[i + 3] & 0xC0) != 0x80) {
            return false;
          }
          uint32_t codepoint = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12)
                             | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
          if (codepoint > 0x10FFFF) {
            return false;  // Unicode range limit
          }
          i += 4;
        } else {
          return false;  // Invalid UTF-8 sequence
        }
      }
      return true;
    }
  }  // namespace

  RawJunction Junction::raw(const crypto::Hasher &hasher) const {
    auto raw = visit_in_place(
        index, [](const auto &x) { return scale::encode(x).value(); });
    common::Hash256 result;
    if (raw.size() > result.size()) {
      result = hasher.blake2b_256(raw);
    } else {
      std::ranges::copy(raw, result.begin());
    }
    return RawJunction{
        .hard = hard,
        .cc = result,
    };
  }

  outcome::result<Mnemonic> Mnemonic::parse(std::string_view phrase) {
    if (!isValidUtf8(std::string(phrase))) {
      return bip39::MnemonicError::INVALID_MNEMONIC;
    }

    Mnemonic mnemonic;

    auto password_pos = phrase.find("///");
    std::string_view seed;
    if (password_pos != std::string_view::npos) {
      // need to normalize password utf8 nfkd
      mnemonic.password = phrase.substr(password_pos + 3);
      seed = phrase.substr(0, password_pos);
    } else {
      seed = phrase;
    }

    auto path = seed;
    auto slash_pos = path.find('/');
    if (slash_pos != std::string_view::npos) {
      seed = seed.substr(0, slash_pos);
      path.remove_prefix(slash_pos);
      while (not path.empty()) {
        path.remove_prefix(1);
        auto hard = not path.empty() and path[0] == '/';
        if (hard) {
          path.remove_prefix(1);
        }
        slash_pos = path.find('/');
        if (slash_pos == std::string_view::npos) {
          slash_pos = path.size();
        }
        auto index = path.substr(0, slash_pos);
        auto end = index.data() + index.size();
        uint64_t num;  // NOLINT(cppcoreguidelines-init-variables)
        auto parse = std::from_chars(index.data(), end, num);
        if (parse.ec == std::errc{} and parse.ptr == end) {
          mnemonic.junctions.emplace_back(Junction{
              .hard = hard,
              .index = num,
          });
        } else {
          mnemonic.junctions.emplace_back(Junction{
              .hard = hard,
              .index = std::string{index},
          });
        }
        path.remove_prefix(slash_pos);
      }
    }

    if (seed.starts_with("0x")) {
      if (seed.size() != HEX_SEED_STR_LENGTH) {
        return bip39::MnemonicError::INVALID_SEED_LENGTH;
      }
      // only half of the buffer will be used
      SecureBuffer<> bytes(Bip39Seed::size(), 0);
      OUTCOME_TRY(common::unhexWith0x(seed, bytes.begin()));
      OUTCOME_TRY(seed, Bip39Seed::from(std::move(bytes)));
      mnemonic.seed = std::move(seed);
    } else {
      Words words;
      if (not seed.empty()) {
        boost::split(words, seed, boost::algorithm::is_space());
      }
      mnemonic.seed = std::move(words);
    }

    return mnemonic;
  }
}  // namespace kagome::crypto::bip39

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto::bip39, MnemonicError, e) {
  switch (e) {
    using enum kagome::crypto::bip39::MnemonicError;
    case INVALID_MNEMONIC:
      return "Mnemonic provided is not valid";
    case INVALID_SEED_LENGTH:
      return "Provided BIP39 seed is not a 64 digit hex number";
  }
  return "unknown MnemonicError";
}
