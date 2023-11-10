/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/uncompress_code_if_needed.hpp"

#include <zstd.h>
#include <zstd_errors.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, UncompressError, e) {
  using E = kagome::runtime::UncompressError;
  switch (e) {
    case E::ZSTD_ERROR:
      return "WASM code not compressed by zstd!";
    case E::BOMB_SIZE_REACHED:
      return "Code decompression failed. Maximum size reached - possible bomb";
  }
  return "Unknown error";
}

namespace kagome::runtime {

  // @see
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/maybe-compressed-blob/src/lib.rs#L28
  constexpr uint8_t kZstdPrefixSize = 8;
  constexpr uint8_t kZstdPrefix[kZstdPrefixSize] = {
      0x52, 0xBC, 0x53, 0x76, 0x46, 0xDB, 0x8E, 0x05};
  // @see
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/maybe-compressed-blob/src/lib.rs#L35
  constexpr size_t kCodeBlobBombLimit = 50 * 1024 * 1024;

  outcome::result<void> uncompressCodeIfNeeded(common::BufferView buf,
                                               common::Buffer &res) {
    if (startsWith(buf, kZstdPrefix)) {
      auto zstd = buf.subspan(std::size(kZstdPrefix));
      // here we can check that blob is really ZSTD compressed
      // but we don't use the result size, it's unknown for the WASM blob
      // @see ZSTD_CONTENTSIZE_UNKNOWN
      auto check_size = ZSTD_getFrameContentSize(zstd.data(), zstd.size());
      if (check_size == ZSTD_CONTENTSIZE_ERROR) {
        return UncompressError::ZSTD_ERROR;
      }
      res.resize(kCodeBlobBombLimit);
      auto size = ZSTD_decompress(
          res.data(), kCodeBlobBombLimit, zstd.data(), zstd.size());
      if (ZSTD_isError(size)) {
        if (ZSTD_getErrorCode(size) == ZSTD_error_dstSize_tooSmall) {
          return UncompressError::BOMB_SIZE_REACHED;
        }
        return UncompressError::ZSTD_ERROR;
      }
      res.resize(size);
      res.shrink_to_fit();
    } else {
      res = common::Buffer{buf};
    }
    return outcome::success();
  }

}  // namespace kagome::runtime
