/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/hasher/hasher_impl.hpp"

#include "gsl/span"

#include "crypto/blake2/blake2b.h"
#include "crypto/sha/sha256.hpp"
#include "crypto/twox/twox.hpp"

namespace kagome::hash {

  HasherImpl::Hash128 HasherImpl::twox_128(
      const HasherImpl::Buffer &buffer) const {
    kagome::common::Blob<16> out;
    crypto::make_twox128(buffer.toBytes(), buffer.size(), out.data());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::twox_256(
      const HasherImpl::Buffer &buffer) const {
    kagome::common::Blob<32> out;
    crypto::make_twox256(buffer.toBytes(), buffer.size(), out.data());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::blake2_256(
      const HasherImpl::Buffer &buffer) const {
    kagome::common::Blob<32> out;
    blake2b(out.data(), 32, nullptr, 0, buffer.toBytes(), buffer.size());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::sha2_256(
      const HasherImpl::Buffer &buffer) const {
    return crypto::sha256(
        {buffer.toBytes(), static_cast<long>(buffer.size())});
  }
}  // namespace kagome::hash
