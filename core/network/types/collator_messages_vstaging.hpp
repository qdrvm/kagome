/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <scale/bitvec.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_types.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/approval/approval.hpp"
#include "parachain/types.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::network::vstaging {
  using Dummy = network::Dummy;
  using Empty = network::Empty;

  using CollatorProtocolMessageDeclare = network::CollatorDeclaration;
  using CollatorProtocolMessageCollationSeconded = network::Seconded;
  using BitfieldDistributionMessage = network::BitfieldDistributionMessage;
  using BitfieldDistribution = network::BitfieldDistribution;
  using ApprovalDistributionMessage = network::ApprovalDistributionMessage;
  using ViewUpdate = network::ViewUpdate;

  struct CollatorProtocolMessageAdvertiseCollation {
    SCALE_TIE(3);
    /// Hash of the relay parent advertised collation is based on.
    RelayHash relay_parent;
    /// Candidate hash.
    CandidateHash candidate_hash;
    /// Parachain head data hash before candidate execution.
    Hash parent_head_data_hash;
  };

  using CollationMessage =
      boost::variant<CollatorProtocolMessageDeclare,
                     CollatorProtocolMessageAdvertiseCollation,
                     Dummy,
                     Dummy,
                     CollatorProtocolMessageCollationSeconded>;

  using CollatorProtocolMessage = boost::variant<CollationMessage>;

  struct SecondedCandidateHash {
    SCALE_TIE(1);
    CandidateHash hash;
  };

  struct ValidCandidateHash {
    SCALE_TIE(1);
    CandidateHash hash;
  };

  /// Statements that can be made about parachain candidates. These are the
  /// actual values that are signed.
  struct CompactStatement {
    std::array<char, 4> header = {'B', 'K', 'N', 'G'};

    boost::variant<Empty,
                   /// Proposal of a parachain candidate.
                   SecondedCandidateHash,
                   /// State that a parachain candidate is valid.
                   ValidCandidateHash>
        inner_value{};

    CompactStatement(
        boost::variant<Empty, SecondedCandidateHash, ValidCandidateHash> &&val)
        : inner_value{std::move(val)} {}
    CompactStatement(ValidCandidateHash &&val) : inner_value{std::move(val)} {}
    CompactStatement(SecondedCandidateHash &&val)
        : inner_value{std::move(val)} {}
    CompactStatement() = default;

    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const CompactStatement &c) {
      s << c.header << c.inner_value;
      return s;
    }

    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, CompactStatement &c) {
      s >> c.header >> c.inner_value;
      return s;
    }

    bool operator==(const CompactStatement &r) const {
      return inner_value == r.inner_value;
    }
  };

  inline const CandidateHash &candidateHash(const CompactStatement &val) {
    auto p = visit_in_place(
        val.inner_value,
        [&](const auto &v)
            -> std::optional<std::reference_wrapper<const CandidateHash>> {
          return {{v.hash}};
        },
        [](const Empty &)
            -> std::optional<std::reference_wrapper<const CandidateHash>> {
          return {};
        });
    if (p) {
      return p->get();
    }
    UNREACHABLE;
  }

  /// A notification of a signed statement in compact form, for a given
  /// relay parent.
  struct StatementDistributionMessageStatement {
    SCALE_TIE(2);

    RelayHash relay_parent;
    IndexedAndSigned<CompactStatement> compact;
  };

  /// All messages for V1 for compatibility with the statement distribution
  /// protocol, for relay-parents that don't support asynchronous backing.
  ///
  /// These are illegal to send to V1 peers, and illegal to send concerning
  /// relay-parents which support asynchronous backing. This backwards
  /// compatibility should be considered immediately deprecated and can be
  /// removed once the node software is not required to support logic from
  /// before asynchronous backing anymore.
  using v1StatementDistributionMessage = network::StatementDistributionMessage;

  enum StatementKind {
    Seconded,
    Valid,
  };

  struct StatementFilter {
    SCALE_TIE(2);

    /// Seconded statements. '1' is known or undesired.
    scale::BitVec seconded_in_group;
    /// Valid statements. '1' is known or undesired.
    scale::BitVec validated_in_group;

    StatementFilter() = default;
    StatementFilter(size_t len, bool val = false) {
      seconded_in_group.bits.assign(len, val);
      validated_in_group.bits.assign(len, val);
    }

    void mask_seconded(const scale::BitVec &mask) {
      for (size_t i = 0; i < seconded_in_group.bits.size(); ++i) {
        const bool m = (i < mask.bits.size()) ? mask.bits[i] : false;
        seconded_in_group.bits[i] = seconded_in_group.bits[i] && !m;
      }
    }

    void mask_valid(const scale::BitVec &mask) {
      for (size_t i = 0; i < validated_in_group.bits.size(); ++i) {
        const bool m = (i < mask.bits.size()) ? mask.bits[i] : false;
        validated_in_group.bits[i] = validated_in_group.bits[i] && !m;
      }
    }

    bool has_len(size_t len) const {
      return seconded_in_group.bits.size() == len
          && validated_in_group.bits.size() == len;
    }

    bool has_seconded() const {
      for (const auto x : seconded_in_group.bits) {
        if (x) {
          return true;
        }
      }
      return false;
    }

    size_t backing_validators() const {
      BOOST_ASSERT(seconded_in_group.bits.size()
                   == validated_in_group.bits.size());

      size_t count = 0;
      for (size_t ix = 0; ix < seconded_in_group.bits.size(); ++ix) {
        const auto s = seconded_in_group.bits[ix];
        const auto v = validated_in_group.bits[ix];
        count += size_t(s || v);
      }
      return count;
    }

    bool contains(size_t index, StatementKind statement_kind) const {
      switch (statement_kind) {
        case StatementKind::Seconded:
          return index < seconded_in_group.bits.size()
              && seconded_in_group.bits[index];
        case StatementKind::Valid:
          return index < validated_in_group.bits.size()
              && validated_in_group.bits[index];
      }
      return false;
    }

    void set(size_t index, StatementKind statement_kind) {
      switch (statement_kind) {
        case StatementKind::Seconded:
          if (index < seconded_in_group.bits.size()) {
            seconded_in_group.bits[index] = true;
          }
          break;
        case StatementKind::Valid:
          if (index < validated_in_group.bits.size()) {
            validated_in_group.bits[index] = true;
          }
          break;
      }
    }
  };

  /// A manifest of a known backed candidate, along with a description
  /// of the statements backing it.
  struct BackedCandidateManifest {
    SCALE_TIE(6);
    /// The relay-parent of the candidate.
    RelayHash relay_parent;
    /// The hash of the candidate.
    CandidateHash candidate_hash;
    /// The group index backing the candidate at the relay-parent.
    GroupIndex group_index;
    /// The para ID of the candidate. It is illegal for this to
    /// be a para ID which is not assigned to the group indicated
    /// in this manifest.
    ParachainId para_id;
    /// The head-data corresponding to the candidate.
    Hash parent_head_data_hash;
    /// A statement filter which indicates which validators in the
    /// para's group at the relay-parent have validated this candidate
    /// and issued statements about it, to the advertiser's knowledge.
    ///
    /// This MUST have exactly the minimum amount of bytes
    /// necessary to represent the number of validators in the assigned
    /// backing group as-of the relay-parent.
    StatementFilter statement_knowledge;
  };

  struct AttestedCandidateRequest {
    SCALE_TIE(2);
    CandidateHash candidate_hash;
    StatementFilter mask;
  };

  struct AttestedCandidateResponse {
    SCALE_TIE(3);
    CommittedCandidateReceipt candidate_receipt;
    runtime::PersistedValidationData persisted_validation_data;
    std::vector<IndexedAndSigned<CompactStatement>> statements;
  };

  /// An acknowledgement of a backed candidate being known.
  struct BackedCandidateAcknowledgement {
    SCALE_TIE(2);
    /// The hash of the candidate.
    CandidateHash candidate_hash;
    /// A statement filter which indicates which validators in the
    /// para's group at the relay-parent have validated this candidate
    /// and issued statements about it, to the advertiser's knowledge.
    ///
    /// This MUST have exactly the minimum amount of bytes
    /// necessary to represent the number of validators in the assigned
    /// backing group as-of the relay-parent.
    StatementFilter statement_knowledge;
  };

  /// Network messages used by the statement distribution subsystem.
  /// Polkadot-sdk analogue:
  /// https://github.com/paritytech/polkadot-sdk/blob/4220503d28f46a72c2bc71f22e7d9708618f9c68/polkadot/node/network/protocol/src/lib.rs#L769
  using StatementDistributionMessage = boost::variant<
      StatementDistributionMessageStatement,  // 0
                                              /// A notification of a backed
                                              /// candidate being known by the
                                              /// sending node, for the purpose
                                              /// of being requested by the
                                              /// receiving node if needed.
      BackedCandidateManifest,  // 1,
      /// A notification of a backed candidate being known by the sending node,
      /// for the purpose of informing a receiving node which already has the
      /// candidate.
      BackedCandidateAcknowledgement  // 2
      // v1StatementDistributionMessage  // 255
      >;

  struct CollationFetchingRequest {
    SCALE_TIE(3);
    /// Relay parent collation is built on top of.
    RelayHash relay_parent;
    /// The `ParaId` of the collation.
    ParachainId para_id;
    /// Candidate hash.
    CandidateHash candidate_hash;
  };

  using CollationFetchingResponse = network::CollationFetchingResponse;

  using ValidatorProtocolMessage = boost::variant<
      Dummy,                         /// NU
      BitfieldDistributionMessage,   /// bitfield distribution message
      Dummy,                         /// NU
      StatementDistributionMessage,  /// statement distribution message
      ApprovalDistributionMessage    /// approval distribution message
      >;

}  // namespace kagome::network::vstaging

namespace kagome::network {

  enum CollationVersion {
    /// The first version.
    V1 = 1,
    /// The staging version.
    VStaging = 2,
  };

  /**
   * Common WireMessage that represents messages in NetworkBridge.
   */
  template <typename T>
  using WireMessage = boost::variant<
      Dummy,  /// not used
      std::enable_if_t<
          AllowerTypeChecker<T,
                             ValidatorProtocolMessage,
                             CollationProtocolMessage,
                             vstaging::ValidatorProtocolMessage,
                             vstaging::CollatorProtocolMessage>::allowed,
          T>,     /// protocol message
      ViewUpdate  /// view update message
      >;

  template <typename V1, typename VStaging>
  using Versioned = boost::variant<V1, VStaging>;

  using VersionedCollatorProtocolMessage =
      Versioned<kagome::network::CollationMessage,
                kagome::network::vstaging::CollatorProtocolMessage>;
  using VersionedValidatorProtocolMessage =
      Versioned<kagome::network::ValidatorProtocolMessage,
                kagome::network::vstaging::ValidatorProtocolMessage>;
  using VersionedStatementDistributionMessage =
      Versioned<kagome::network::StatementDistributionMessage,
                kagome::network::vstaging::StatementDistributionMessage>;
}  // namespace kagome::network
