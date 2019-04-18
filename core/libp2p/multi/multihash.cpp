/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multihash.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>
#include "common/hexutil.hpp"
#include "libp2p/multi/uvarint.hpp"


using kagome::common::Buffer;
using kagome::common::unhex;

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, Multihash::Error, e) {
  using E = libp2p::multi::Multihash::Error;
  switch (e) {
    case E::ZERO_INPUT_LENGTH:
      return "The length encoded in the header is zero";
    case E::INCONSISTENT_LENGTH:
      return "The length encoded in the input data header doesn't match the actual length of the input data";
    case E::INPUT_TOO_LONG:
      return "The length of the input exceeds the maximum length of " + std::to_string(libp2p::multi::Multihash::kMaxHashLength);
    case E::INPUT_TOO_SHORT:
      return "The length of the input is less than the required minimum of two bytes for the multihash header";
    default:
      return "Unknown error";
  }
}

namespace libp2p::multi {

  Multihash::Multihash(HashType type, Hash hash)
      : hash_{std::move(hash)}, type_{type} {
    UVarint uvarint{type};
    data_.put(uvarint.toBytes());
    data_.putUint8(static_cast<uint8_t>(hash_.size()));
    data_.put(hash_.toVector());
  }

  auto Multihash::create(HashType type, Hash hash)
      -> outcome::result<Multihash> {
    if (hash.size() > kMaxHashLength) {
      return Error::INPUT_TOO_LONG;
    }

    return Multihash{type, std::move(hash)};
  }

  auto Multihash::createFromHex(std::string_view hex)
      -> outcome::result<Multihash> {
    OUTCOME_TRY(res, Buffer::fromHex(hex));
    return Multihash::createFromBuffer(res);
  }

  auto Multihash::createFromBuffer(const Buffer &b)
      -> outcome::result<Multihash> {
    if (b.size() < kHeaderSize) {
      return Error::INPUT_TOO_SHORT;
    }

    UVarint varint(gsl::span(b.toBytes(), b.size()));

    const auto type = static_cast<HashType>(varint.toUInt64());
    uint8_t length = b[varint.size()];
    Hash hash(std::vector<uint8_t>(b.begin() + varint.size() + 1, b.end()));

    if (length == 0) {
      return Error::ZERO_INPUT_LENGTH;
    }

    if (hash.size() != length) {
      auto s =
          "The hash length %1%, provided in the header, "
          "does not equal the actual hash length %2%";
      return Error::INCONSISTENT_LENGTH;
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

  bool Multihash::operator==(const Multihash &other) const {
    return this->data_ == other.data_ && this->type_ == other.type_;
  }

}  // namespace libp2p::multi
