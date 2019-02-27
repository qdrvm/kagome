/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase.hpp"

namespace {
  using Encoding = libp2p::multi::Multibase::Encoding;

  std::string encodeData(std::string_view data, Encoding base) {
    return codec(base).encode(data, false);
  }

  std::string decodeData(std::string_view data, Encoding base) {
    return codec(base).decode(data);
  }
}  // namespace

namespace libp2p::multi {
  using namespace kagome::expected;

  Multibase::FactoryResult Multibase::createMultibaseFromEncoded(
      std::string_view encoded_data) {
    if (encoded_data.length() < 2) {
      return Error{"multibase must be at least 2 characters"};
    }
    auto base = static_cast<Encoding>(encoded_data[0]);
    auto raw_data = decodeData(encoded_data.substr(1), base);
    return Value{
        Multibase{std::string{encoded_data}, std::move(raw_data), base}};
  }

  Multibase::FactoryResult Multibase::createMultibaseFromRaw(
      std::string_view raw_data, Encoding base) {
    if (raw_data.empty()) {
      return Error{"empty string provided"};
    }
    auto encoded_data = encodeData(raw_data, base);
    return Value{
        Multibase{std::move(encoded_data), std::string{raw_data}, base}};
  }

  Multibase::Multibase(std::string &&encoded_data,
                       std::string &&raw_data,
                       Encoding base)
      : encoded_data_{std::move(encoded_data)},
        raw_data_{std::move(raw_data)},
        base_{base} {}

  Multibase::Encoding Multibase::base() const {
    return base_;
  }

  std::string_view Multibase::encodedData() const {
    return encoded_data_;
  }

  std::string_view Multibase::rawData() const {
    return raw_data_;
  }

  bool Multibase::operator==(const Multibase &rhs) const {
    return this->encoded_data_ == rhs.encoded_data_
        && this->raw_data_ == rhs.raw_data_ && this->base_ == rhs.base_;
  }

  bool Multibase::operator!=(const Multibase &rhs) const {
    return !(*this == rhs);
  }
}  // namespace libp2p::multi
