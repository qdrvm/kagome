/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/mnemonic.hpp"

#include <charconv>
#include <codecvt>
#include <locale>

#include <boost/algorithm/string.hpp>

#include "common/visitor.hpp"
#include "crypto/bip39/bip39_types.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"

namespace kagome::crypto::bip39 {

  namespace {
    /**
     * @return true if string s is a valid utf-8, false otherwise
     */
    bool isValidUtf8(const std::string &s) {
      // cannot replace for std::string_view for security reasons
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
          utf16conv;
      try {
        auto utf16 = utf16conv.from_bytes(s.data());
      } catch (...) {
        return false;
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
      std::copy(raw.begin(), raw.end(), result.begin());
    }
    return RawJunction{hard, result};
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
        uint64_t num;
        auto parse = std::from_chars(index.data(), end, num);
        if (parse.ec == std::errc{} and parse.ptr == end) {
          mnemonic.junctions.emplace_back(Junction{hard, num});
        } else {
          mnemonic.junctions.emplace_back(Junction{hard, std::string{index}});
        }
        path.remove_prefix(slash_pos);
      }
    }

    if (seed.starts_with("0x")) {
      OUTCOME_TRY(bytes, common::unhexWith0x(seed));
      mnemonic.seed = common::Buffer{std::move(bytes)};
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
  using Error = kagome::crypto::bip39::MnemonicError;
  switch (e) {
    case Error::INVALID_MNEMONIC:
      return "Mnemonic provided is not valid";
  }
  return "unknown MnemonicError";
}
