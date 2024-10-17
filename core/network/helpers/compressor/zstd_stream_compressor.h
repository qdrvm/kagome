/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "compressor.h"
namespace kagome::network {
struct ZstdStreamCompressor : public ICompressor {
    ZstdStreamCompressor(int compressionLevel = 3) : m_compressionLevel(compressionLevel) {}
    outcome::result<std::vector<uint8_t>> compress(std::span<uint8_t> data) override;
    outcome::result<std::vector<uint8_t>> decompress(std::span<uint8_t> compressedData) override;
private:
    int m_compressionLevel;
};

}  // namespace kagome::network
