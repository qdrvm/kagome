/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HASH_TYPE_HPP
#define KAGOME_HASH_TYPE_HPP

namespace libp2p::multi {
  enum HashType : uint64_t {
    kSha1 = 0x11,
    kSha256 = 0x12,
    kSha512 = 0x13,
    kBlake2s128 = 0xb250,
    kBlake2s256 = 0xb260
  };
}

#endif  // KAGOME_HASH_TYPE_HPP
