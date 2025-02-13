/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/container_hash/hash.hpp>

#include "common/unused.hpp"
#include "consensus/beefy/types/authority.hpp"
#include "crypto/ecdsa_types.hpp"
#include "primitives/common.hpp"

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
    std::vector<crypto::EcdsaPublicKey> validators;
    AuthoritySetId id = 0;

    std::optional<AuthorityIndex> find(
        const crypto::EcdsaPublicKey &key) const {
      auto it = std::find(validators.begin(), validators.end(), key);
      if (it == validators.end()) {
        return std::nullopt;
      }
      return it - validators.begin();
    }
  };

  using ConsensusDigest = boost::variant<Unused<0>,
                                         ValidatorSet,    // AuthoritiesChange
                                         AuthorityIndex,  // OnDisabled
                                         MmrRootHash>;

  using PayloadId = common::Blob<2>;
  constexpr PayloadId kMmr{{'m', 'h'}};

  struct Commitment {
    std::vector<std::pair<PayloadId, common::Buffer>> payload;
    primitives::BlockNumber block_number;
    AuthoritySetId validator_set_id;
    bool operator==(const Commitment &other) const = default;
  };

  struct VoteMessage {
    Commitment commitment;
    crypto::EcdsaPublicKey id;
    crypto::EcdsaSignature signature;
  };

  struct SignedCommitment {
    Commitment commitment;
    std::vector<std::optional<crypto::EcdsaSignature>> signatures;
  };

  inline void encode(const SignedCommitment &v, scale::Encoder &encoder) {
    encode(v.commitment, encoder);
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
    encode(bits, encoder);
    encode(static_cast<uint32_t>(v.signatures.size()), encoder);
    encode(scale::as_compact(count), encoder);
    for (auto &sig : v.signatures) {
      if (sig) {
        encode(*sig, encoder);
      }
    }
  }

  inline void decode(SignedCommitment &v, scale::Decoder &decoder) {
    decode(v.commitment, decoder);
    common::Buffer bits;
    decode(bits, decoder);
    size_t expected_count = 0;
    for (auto byte : bits) {
      for (; byte; byte >>= 1) {
        expected_count += byte & 1;
      }
    }
    uint32_t total = 0;
    decode(total, decoder);
    if (bits.size() * 8 < total) {
      scale::raise(scale::DecodeError::NOT_ENOUGH_DATA);
    }
    size_t actual_count;
    decode(scale::as_compact(actual_count), decoder);
    if (actual_count != expected_count) {
      scale::raise(scale::DecodeError::TOO_MANY_ITEMS);
    }
    v.signatures.resize(total);
    for (size_t i = 0; i < total; ++i) {
      if ((bits[i / 8] & (1 << (7 - i % 8))) != 0) {
        decode(v.signatures[i].emplace(), decoder);
      }
    }
  }

  using BeefyJustification = boost::variant<Unused<0>, SignedCommitment>;

  using BeefyGossipMessage = boost::variant<VoteMessage, BeefyJustification>;

  struct DoubleVotingProof {
    VoteMessage first;
    VoteMessage second;
  };
}  // namespace kagome::consensus::beefy

template <>
struct std::hash<kagome::consensus::beefy::Commitment> {
  size_t operator()(const kagome::consensus::beefy::Commitment &v) const {
    size_t h = 0;
    boost::hash_combine(h, v.validator_set_id);
    boost::hash_combine(h, v.block_number);
    for (auto &p : v.payload) {
      boost::hash_combine(h, p.first);
      boost::hash_combine(h, p.second);
    }
    return h;
  }
};
