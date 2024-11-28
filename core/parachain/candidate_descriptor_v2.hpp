/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/endian/conversion.hpp>

#include "crypto/sr25519_provider.hpp"
#include "parachain/pvf/pvf_error.hpp"
#include "parachain/transpose_claim_queue.hpp"
#include "parachain/types.hpp"
#include "parachain/ump_signal.hpp"

namespace kagome::network {
  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L44
  /// The default claim queue offset to be used if it's not
  /// configured/accessible in the parachain
  /// runtime
  constexpr uint8_t kDefaultClaimQueueOffset = 0;

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L532-L534
  inline bool isV1(const CandidateDescriptor &descriptor) {
    constexpr auto is_zero = [](BufferView xs) {
      return static_cast<size_t>(std::ranges::count(xs, 0)) == xs.size();
    };
    return not is_zero(std::span{descriptor.reserved_1}.subspan(7))
        or not is_zero(descriptor.reserved_2);
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L532-L537
  inline bool isV2(const CandidateDescriptor &descriptor) {
    return not isV1(descriptor) and descriptor.reserved_1[0] == 0;
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L179
  /// Check the signature of the collator within this descriptor.
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

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L580
  /// Returns the `core_index` of `V2` candidate descriptors, `None` otherwise.
  inline std::optional<parachain::CoreIndex> coreIndex(
      const CandidateDescriptor &descriptor) {
    if (isV1(descriptor)) {
      return std::nullopt;
    }
    return boost::endian::load_little_u16(&descriptor.reserved_1[1]);
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L589
  /// Returns the `core_index` of `V2` candidate descriptors, `None` otherwise.
  inline std::optional<parachain::SessionIndex> sessionIndex(
      const CandidateDescriptor &descriptor) {
    if (isV1(descriptor)) {
      return std::nullopt;
    }
    return boost::endian::load_little_u32(&descriptor.reserved_1[3]);
  }

  enum class CheckCoreIndexError : uint8_t {
    InvalidCoreIndex,
    NoAssignment,
    UnknownVersion,
    InvalidSession,
  };
  Q_ENUM_ERROR_CODE(CheckCoreIndexError) {
    using E = decltype(e);
    switch (e) {
      case E::InvalidCoreIndex:
        return "The specified core index is invalid";
      case E::NoAssignment:
        return "The parachain is not assigned to any core at specified claim "
               "queue offset";
      case E::UnknownVersion:
        return "Unknown internal version";
      case E::InvalidSession:
        return "Invalid session";
    }
    abort();
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L602
  /// Checks if descriptor core index is equal to the committed core index.
  /// Input `cores_per_para` is a claim queue snapshot at the candidate's relay
  /// parent, stored as a mapping between `ParaId` and the cores assigned per
  /// depth.
  inline outcome::result<void> checkCoreIndex(
      const CommittedCandidateReceipt &receipt,
      const TransposedClaimQueue &claims) {
    if (isV1(receipt.descriptor)) {
      return outcome::success();
    }
    if (not isV2(receipt.descriptor)) {
      return CheckCoreIndexError::UnknownVersion;
    }
    OUTCOME_TRY(selector, coreSelector(receipt.commitments));
    auto offset =
        selector ? selector->claim_queue_offset : kDefaultClaimQueueOffset;
    auto it1 = claims.find(receipt.descriptor.para_id);
    if (it1 == claims.end()) {
      return CheckCoreIndexError::NoAssignment;
    }
    auto it2 = it1->second.find(offset);
    if (it2 == it1->second.end()) {
      return CheckCoreIndexError::NoAssignment;
    }
    auto &assigned_cores = it2->second;
    if (assigned_cores.empty()) {
      return CheckCoreIndexError::NoAssignment;
    }
    auto core = coreIndex(receipt.descriptor).value();
    if (not selector and assigned_cores.size() > 1) {
      if (not assigned_cores.contains(core)) {
        return CheckCoreIndexError::InvalidCoreIndex;
      }
      return outcome::success();
    }
    auto expected_core =
        *std::next(assigned_cores.begin(),
                   selector ? static_cast<ptrdiff_t>(selector->core_selector
                                                     % assigned_cores.size())
                            : 0);
    if (core != expected_core) {
      return CheckCoreIndexError::InvalidCoreIndex;
    }
    return outcome::success();
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/node/network/collator-protocol/src/validator_side/mod.rs#L2114
  inline outcome::result<void> descriptorVersionSanityCheck(
      const CandidateDescriptor &descriptor,
      bool v2_receipts,
      CoreIndex expected_core,
      SessionIndex expected_session) {
    if (isV1(descriptor)) {
      return outcome::success();
    }
    if (not isV2(descriptor)) {
      return CheckCoreIndexError::UnknownVersion;
    }
    if (not v2_receipts) {
      return CheckCoreIndexError::UnknownVersion;
    }
    if (coreIndex(descriptor) != expected_core) {
      return CheckCoreIndexError::InvalidCoreIndex;
    }
    if (sessionIndex(descriptor) != expected_session) {
      return CheckCoreIndexError::InvalidSession;
    }
    return outcome::success();
  }
}  // namespace kagome::network
