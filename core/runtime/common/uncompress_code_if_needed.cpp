/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/uncompress_code_if_needed.hpp"

#include <zstd.h>

namespace kagome::runtime {

  // @see
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/maybe-compressed-blob/src/lib.rs#L28
  constexpr uint8_t kZstdPrefix[8] = {
      0x52, 0xBC, 0x53, 0x76, 0x46, 0xDB, 0x8E, 0x05};
  // @see
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/maybe-compressed-blob/src/lib.rs#L35
  constexpr size_t kCodeBlobBombLimit = 50 * 1024 * 1024;

  void uncompressCodeIfNeeded(const common::Buffer &buf, common::Buffer &res) {
    if (std::equal(buf.begin(),
                   buf.begin() + 8,
                   std::begin(kZstdPrefix),
                   std::end(kZstdPrefix))) {
      res.resize(kCodeBlobBombLimit);
      auto size = ZSTD_decompress(
          res.data(), kCodeBlobBombLimit, buf.data() + 8, buf.size() - 8);
      res.resize(size);
    } else {
      res = buf;
    }
  }

}  // namespace kagome::runtime
