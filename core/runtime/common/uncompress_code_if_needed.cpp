/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/uncompress_code_if_needed.hpp"

#include <zstd.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, UncompressError, e) {
  using E = kagome::runtime::UncompressError;
  switch (e) {
    case E::ZSTD_ERROR:
      return "WASM code not compressed by zstd!";
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

  outcome::result<void, UncompressError> uncompressCodeIfNeeded(
      common::BufferView buf, common::Buffer &res) {
    if (buf.size() > kZstdPrefixSize
        && std::equal(buf.begin(),
                      buf.begin() + kZstdPrefixSize,
                      std::begin(kZstdPrefix),
                      std::end(kZstdPrefix))) {
      // here we can check that blob is really ZSTD compressed
      // but we don't use the result size, it's unknown for the WASM blob
      // @see ZSTD_CONTENTSIZE_UNKNOWN
      auto check_size = ZSTD_getFrameContentSize(buf.data() + kZstdPrefixSize,
                                                 buf.size() - kZstdPrefixSize);
      if (check_size == ZSTD_CONTENTSIZE_ERROR) {
        return UncompressError::ZSTD_ERROR;
      }
      res.resize(kCodeBlobBombLimit);
      auto size = ZSTD_decompress(res.data(),
                                  kCodeBlobBombLimit,
                                  buf.data() + kZstdPrefixSize,
                                  buf.size() - kZstdPrefixSize);
      res.resize(size);
    } else {
      res = common::Buffer{buf};
    }
    return outcome::success();
  }

}  // namespace kagome::runtime
