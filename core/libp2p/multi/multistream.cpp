/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multistream.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string_view>

#include "libp2p/multi/uvarint.hpp"

using kagome::common::Buffer;
using kagome::expected::Error;
using kagome::expected::Result;
using kagome::expected::Value;
using libp2p::multi::UVarint;

namespace libp2p::multi {

  Multistream::Multistream(Path codecPath, const Buffer &data)
      : protocol_path_{std::move(codecPath)} {
    initBuffer(protocol_path_, data.toVector());
    initData();
  }

  auto Multistream::create(Path protocol_path, const Buffer &data)
      -> Result<Multistream, std::string> {
    if (protocol_path.find('\n') != std::string::npos) {
      return Error{"Codec path must not contain the new line symbol"};
    }
    if (protocol_path.front() != '/') {
      return Error{"Codec path must begin with '/'"};
    }
    return Value{Multistream{std::move(protocol_path), data}};
  }

  auto Multistream::create(const Buffer &bytes)
      -> Result<Multistream, std::string> {
    size_t varint_length = UVarint::calculateSize(bytes.toVector());
    UVarint size =
        UVarint(gsl::span<const uint8_t>(bytes.toBytes(), varint_length));

    auto path_end = std::find(bytes.begin() + varint_length, bytes.end(), '\n');
    if (path_end == bytes.end()) {
      return Error{"New line delimiter is not found"};
    }
    if (std::distance(bytes.begin() + varint_length, bytes.end())
        != size.toUInt64()) {
      return Error{
          "Data size specified in the multistream header does not equal the "
          "actual size"};
    }

    Multistream m;
    std::copy(bytes.begin() + varint_length, path_end,
              std::back_inserter(m.protocol_path_));
    m.multistream_buffer_ = bytes;
    m.initData();
    return Value{std::move(m)};
  }

  auto Multistream::addPrefix(std::string_view prefix)
      -> Result<std::reference_wrapper<Multistream>, std::string> {
    if (prefix.empty() || prefix.find("\n") != std::string::npos
        || prefix.find("/") != std::string::npos) {
      return Error{
          "Prefix must not contain line breaks or forward slashes, as it is a "
          "part of the protocol URI"};
    }
    protocol_path_.insert(std::begin(protocol_path_), prefix.begin(),
                          prefix.end());
    protocol_path_.insert(std::begin(protocol_path_), '/');
    auto new_length = UVarint(prefix.length() + 1 + data_.size());

    initBuffer(protocol_path_, data_);
    initData();
    return Value{(*this)};
  }

  auto Multistream::removePrefix(std::string_view prefix)
      -> kagome::expected::Result<std::reference_wrapper<Multistream>,
                                  std::string> {
    if (prefix.empty() or prefix.find("\n") != std::string::npos
        or prefix.find("/") != std::string::npos) {
      return Error{
          "Prefix must not contain line breaks or forward slashes, as it is a "
          "part of the protocol URI"};
    }
    auto prefix_pos = protocol_path_.find(prefix);

    if (prefix_pos == std::string::npos) {
      std::stringstream ss;
      ss << "The prefix " << prefix << " is not found in the protocol path "
         << protocol_path_;
      return Error{ss.str()};
    }

    if (prefix.size() == protocol_path_.size() - 1) {
      return Error{
          "Attempt to remove the only part of the path; Empty protocol path is "
          "prohibited"};
    }

    // -1/+1 because / preceding the prefix must also be removed
    protocol_path_.erase(prefix_pos - 1, prefix.size() + 1);

    initBuffer(protocol_path_, data_);
    initData();
    return Value{(*this)};
  }

  const Multistream::Path &Multistream::getProtocolPath() const {
    return protocol_path_;
  }

  gsl::span<const uint8_t> Multistream::getEncodedData() const {
    return data_;
  }

  const Buffer &Multistream::getBuffer() const {
    return multistream_buffer_;
  }

  void Multistream::initBuffer(std::string_view protocol_path,
                               gsl::span<const uint8_t> data) {
    auto new_length = UVarint(protocol_path.length() + 1 + data.size());

    multistream_buffer_ = Buffer()
                              .put(new_length.toBytes())
                              .put(protocol_path)
                              .putUint8('\n')
                              .put(data);
  }

  void Multistream::initData() {
    auto length = UVarint(protocol_path_.length() + 1 + data_.size());

    auto data_begin = length.size() + protocol_path_.length() + 1;
    auto it = multistream_buffer_.begin();
    std::advance(it, data_begin);
    data_ = gsl::span<const uint8_t>(it.base(),
                                     multistream_buffer_.size() - data_begin);
  }

  bool Multistream::operator==(const Multistream &other) const {
    return this->multistream_buffer_ == other.multistream_buffer_;
  }

}  // namespace libp2p::multi
