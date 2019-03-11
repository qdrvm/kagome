/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multistream.hpp"

#include <iterator>
#include <string_view>

using kagome::common::Buffer;
using kagome::expected::Error;
using kagome::expected::Result;
using kagome::expected::Value;

namespace libp2p::multi {

  Multistream::Multistream(std::filesystem::path codecPath, Buffer data)
      : multistream_buffer_{} {
    auto path_str = codecPath.string();
    multistream_buffer_
        // varint length, which includes path + '\n' + data size
        .put(toVarInt(path_str.length() + 1 + data.size()))
        .put_uint8('/')
        .put(path_str)
        .put_uint8('\n')
        .put(data.to_vector());
  }

  auto Multistream::create(std::filesystem::path codecPath, Buffer data)
      -> Result<Multistream, std::invalid_argument> {
    return Value{Multistream{std::move(codecPath), std::move(data)}};
  }

  const std::filesystem::path &Multistream::getCodecPath() const {
    return codec_path_;
  }

  const Buffer &Multistream::getEncodedData() const {
    return encoded_data_;
  }

  const Buffer &Multistream::getBuffer() const {
    return multistream_buffer_;
  }

  std::vector<uint8_t> Multistream::toVarInt(size_t n) {
    uint8_t buf[8];

    

    auto actual_size = uvarint_encode64(n, buf, 8);
    return std::vector(buf, buf + actual_size);
  }

}  // namespace libp2p::multi
