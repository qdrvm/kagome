/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/unused.hpp"
#include "crypto/ecdsa_types.hpp"
#include "primitives/authority.hpp"

/**
 * Test
 *   ConsensusDigest
 *     0x0108020a1091341fe5664bfa1782d5e04779689068c916b04cb365ec3153755684d9a10390084fdbf27d2b79d26a4f13f0ccd982cb755a661969143c37cbc49ef5b91f270700000000000000
 *   BeefyJustification
 *     0x01046d68803af1ad0102f711a7c08e589a1006e4f20c8853b12b5214a57a08cbb4c72cf2ce47000000070000000000000004c002000000080e0fa849fcd9ecfed1b1312e7a17bb4db4ec02761ac760b01a9fc7365c2f55a059125b6217943b561aa27c8b1f990eee1cc9b72ff6f4d6ddde467e33dd02142500f016a7aa597346546f0e799016c8a5302c7a6dce286c513bd69c60e1e77b1e2f6bff5c269369b4ede6fd6e41b32186faff8773158708b16a35d2afcdc9aeeaa500
 */

namespace kagome::consensus::beefy {
  using MmrRootHash = common::Hash256;

  struct ValidatorSet {
    SCALE_TIE(2);

    std::vector<crypto::EcdsaPublicKey> validators;
    primitives::AuthoritySetId id;
  };

  using ConsensusDigest =
      boost::variant<Unused<0>,
                     ValidatorSet,                // AuthoritiesChange
                     primitives::AuthorityIndex,  // OnDisabled
                     MmrRootHash>;

  using PayloadId = common::Blob<2>;

  struct Commitment {
    SCALE_TIE(3);

    std::vector<std::pair<PayloadId, common::Buffer>> payload;
    primitives::BlockNumber block_number;
    primitives::AuthoritySetId validator_set_id;
  };

  struct VoteMessage {
    SCALE_TIE(3);

    Commitment commitment;
    crypto::EcdsaPublicKey id;
    crypto::EcdsaSignature signature;
  };

  struct SignedCommitment {
    Commitment commitment;
    std::vector<std::optional<crypto::EcdsaSignature>> signatures;
  };
  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const SignedCommitment &v) {
    s << v.commitment;
    size_t count = 0;
    common::Buffer bits;
    // https://github.com/paritytech/substrate/blob/55bb6298e74d86be12732fd0f120185ee8fbfe97/primitives/consensus/beefy/src/commitment.rs#L149-L152
    bits.resize(v.signatures.size() / 8 + 1);
    auto i = 0;
    for (auto &sig : v.signatures) {
      if (sig) {
        ++count;
        bits[i / 8] |= 1 << (7 - i % 8);
      }
      ++i;
    }
    s << bits;
    s << static_cast<uint32_t>(v.signatures.size());
    s << scale::CompactInteger{count};
    for (auto &sig : v.signatures) {
      if (sig) {
        s << *sig;
      }
    }
    return s;
  }
  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        SignedCommitment &v) {
    s >> v.commitment;
    common::Buffer bits;
    s >> bits;
    size_t expected_count = 0;
    for (auto byte : bits) {
      for (; byte; byte >>= 1) {
        expected_count += byte & 1;
      }
    }
    uint32_t total = 0;
    s >> total;
    if (bits.size() * 8 < total) {
      scale::raise(scale::DecodeError::NOT_ENOUGH_DATA);
    }
    scale::CompactInteger actual_count;
    s >> actual_count;
    if (actual_count != expected_count) {
      scale::raise(scale::DecodeError::TOO_MANY_ITEMS);
    }
    v.signatures.resize(total);
    for (size_t i = 0; i < total; ++i) {
      if ((bits[i / 8] & (1 << (7 - i % 8))) != 0) {
        s >> v.signatures[i].emplace();
      }
    }
    return s;
  }

  using BeefyJustification = boost::variant<Unused<0>, SignedCommitment>;
}  // namespace kagome::consensus::beefy
