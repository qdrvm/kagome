/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multihash.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>
#include "common/hexutil.hpp"
#include "libp2p/multi/uvarint.hpp"

using kagome::expected::Error;
using kagome::expected::Result;
using kagome::expected::Value;

using kagome::common::Buffer;
using kagome::common::unhex;

namespace libp2p::multi {

  Multihash::Multihash(HashType type, Hash hash)
      : hash_{std::move(hash)}, type_{type} {
      UVarint uvaring{type};
      data_.put(uvaring.toBytes());
      data_.putUint8(static_cast<uint8_t>(hash_.size()));
      data_.put(hash_.toVector());
  }

  auto Multihash::create(HashType type, Hash hash)
      -> Result<Multihash, std::string> {
    if (hash.size() > kMaxHashLength) {
      constexpr auto s =
          "The hash is too long: %1% bytes against maximum of %2% bytes";
      return Error{str(boost::format(s) % hash.size() % kMaxHashLength)};
    }

    return Value{Multihash{type, std::move(hash)}};
  }

  auto Multihash::createFromHex(std::string_view hex)
      -> Result<Multihash, std::string> {
    return Buffer::fromHex(hex).match(
        [](const Value<Buffer> &v) { return Multihash::createFromBuffer(v.value); },
        [&hex](const Error<std::string>& e) -> Result<Multihash, std::string> {
          return Error{std::string(hex) + " is not a valid hex string; Error: " + e.error};
        });
  }

  auto Multihash::createFromBuffer(const Buffer &b)
      -> Result<Multihash, std::string> {
    if (b.size() < kHeaderSize) {
      return Error{
          "Buffer is too short; Need at least include 2 byte header with hash "
          "type and length"};
    }

    UVarint varint(gsl::span(b.toBytes(), b.size()));

    const auto type = static_cast<HashType>(varint.toUInt64());
    uint8_t length = b[varint.size()];
    Hash hash(std::vector<uint8_t>(b.begin() + varint.size() + 1, b.end()));

    if (length == 0) {
      return Error{"The length is zero, which is not a valid hash"};
    }

    if (hash.size() != length) {
      auto s =
          "The hash length %1%, provided in the header, "
          "does not equal the actual hash length %2%";
      return Error{str(boost::format(s) % length % hash.size())};
    }

    return Multihash::create(type, std::move(hash));
  }

  const HashType &Multihash::getType() const {
    return type_;
  }

  const Multihash::Hash &Multihash::getHash() const {
    return hash_;
  }

  std::string Multihash::toHex() const {
    return data_.toHex();
  }

  const Buffer &Multihash::toBuffer() const {
    return data_;
  }

}  // namespace libp2p::multi
