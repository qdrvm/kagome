/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/twox/twox.hpp"

namespace kagome::hash {

  HasherImpl::Hash128 HasherImpl::hashTwox_128(
      const HasherImpl::Buffer &buffer) {
    using Blob = kagome::common::Blob<16>;
    Blob out;
    crypto::make_twox128(buffer.toBytes(), buffer.size(), out.data());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::hashTwox_256(
      const HasherImpl::Buffer &buffer) {
    using Blob = kagome::common::Blob<32>;
    Blob out;
    crypto::make_twox256(buffer.toBytes(), buffer.size(), out.data());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::hashBlake2_256(
      const HasherImpl::Buffer &buffer) {
    std::terminate();
  }

  HasherImpl::Hash256 HasherImpl::hashSha2_256(
      const HasherImpl::Buffer &buffer) {
    std::terminate();
  }
}  // namespace kagome::hash
