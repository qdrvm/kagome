/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/uncompress_code_if_needed.hpp"

#include <zstd.h>

#include "log/logger.hpp"

namespace kagome::runtime {

  // @see
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/maybe-compressed-blob/src/lib.rs#L28
  constexpr uint8_t kZstdPrefixSize = 8;
  constexpr uint8_t kZstdPrefix[kZstdPrefixSize] = {
      0x52, 0xBC, 0x53, 0x76, 0x46, 0xDB, 0x8E, 0x05};
  // @see
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/maybe-compressed-blob/src/lib.rs#L35
  constexpr size_t kCodeBlobBombLimit = 50 * 1024 * 1024;

  void uncompressCodeIfNeeded(const common::Buffer &buf, common::Buffer &res) {
    if (buf.size() > kZstdPrefixSize
        && std::equal(buf.begin(),
                      buf.begin() + kZstdPrefixSize,
                      std::begin(kZstdPrefix),
                      std::end(kZstdPrefix))) {
      auto logger = log::createLogger("UncompressCodeIfNeeded", "wasm");
      // here we can check that blob is really ZSTD compressed
      // but we don't use the result size, it's unknown for the WASM blob
      // @see ZSTD_CONTENTSIZE_UNKNOWN
      auto check_size = ZSTD_getFrameContentSize(buf.data() + kZstdPrefixSize,
                                                 buf.size() - kZstdPrefixSize);
      // error here would mean an update of codebase
      // so we leave it to try the previous runtime if any
      if (check_size == ZSTD_CONTENTSIZE_ERROR) {
        logger->error("WASM code not compressed by zstd!");
        return;
      }
      res.resize(kCodeBlobBombLimit);
      auto size = ZSTD_decompress(res.data(),
                                  kCodeBlobBombLimit,
                                  buf.data() + kZstdPrefixSize,
                                  buf.size() - kZstdPrefixSize);
      res.resize(size);
    } else {
      res = buf;
    }
  }

}  // namespace kagome::runtime
