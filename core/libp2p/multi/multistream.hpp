/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISTREAM_HPP
#define KAGOME_MULTISTREAM_HPP

#include <filesystem>

#include "common/buffer.hpp"
#include "common/result.hpp"

namespace libp2p::multi {
  /**
   * Format of stream identifier used in Libp2p
   */
  class Multistream {

  public:

    static auto create(std::filesystem::path codecPath,
                       kagome::common::Buffer data)
                       -> kagome::expected::Result<Multistream, std::invalid_argument>;

    auto getCodecPath() const -> const std::filesystem::path&;
    auto getEncodedData() const -> const kagome::common::Buffer&;
    auto getBuffer() const -> const kagome::common::Buffer&;

  private:
    Multistream(std::filesystem::path codecPath,
                kagome::common::Buffer data);

    static std::vector<uint8_t> toVarInt(size_t n);

    std::filesystem::path codec_path_;
    kagome::common::Buffer encoded_data_;
    kagome::common::Buffer multistream_buffer_;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTISTREAM_HPP
