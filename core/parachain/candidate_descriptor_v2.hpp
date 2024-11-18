/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/endian/conversion.hpp>

#include "crypto/sr25519_provider.hpp"
#include "parachain/pvf/pvf_error.hpp"
#include "parachain/types.hpp"

namespace kagome::network {
  inline bool isV1(const CandidateDescriptor &descriptor) {
    constexpr auto is_zero = [](BufferView xs) {
      return static_cast<size_t>(std::ranges::count(xs, 0)) == xs.size();
    };
    return not is_zero(std::span{descriptor.reserved_1}.subspan(7))
        or not is_zero(descriptor.reserved_2);
  }

  inline bool isV2(const CandidateDescriptor &descriptor) {
    return not isV1(descriptor) and descriptor.reserved_1[0] == 0;
  }

  inline outcome::result<void> checkSignature(
      const crypto::Sr25519Provider &sr25519,
      const CandidateDescriptor &descriptor) {
    if (not isV1(descriptor)) {
      return outcome::success();
    }
    OUTCOME_TRY(
        r,
        sr25519.verify(crypto::Sr25519Signature{descriptor.reserved_2},
                       descriptor.signable(),
                       crypto::Sr25519PublicKey{descriptor.reserved_1}));
    if (not r) {
      return PvfError::SIGNATURE;
    }
    return outcome::success();
  }

  inline std::optional<parachain::CoreIndex> coreIndex(
      const CandidateDescriptor &descriptor) {
    if (isV1(descriptor)) {
      return std::nullopt;
    }
    return boost::endian::load_little_u16(&descriptor.reserved_1[1]);
  }

  inline std::optional<parachain::SessionIndex> sessionIndex(
      const CandidateDescriptor &descriptor) {
    if (isV1(descriptor)) {
      return std::nullopt;
    }
    return boost::endian::load_little_u32(&descriptor.reserved_1[3]);
  }
}  // namespace kagome::network
