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

  struct CompactStatement {
    std::array<char, 4> header = {'B', 'K', 'N', 'G'};
    boost::variant<Empty, SecondedCandidateHash, ValidCandidateHash>
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

  struct StatementDistributionMessageStatement {
    SCALE_TIE(2);

    RelayHash relay_parent;
    IndexedAndSigned<CompactStatement> compact;
  };

  using v1StatementDistributionMessage = network::StatementDistributionMessage;

  struct StatementFilter {
    SCALE_TIE(2);

    /// Seconded statements. '1' is known or undesired.
    scale::BitVec seconded_in_group;
    /// Valid statements. '1' is known or undesired.
    scale::BitVec validated_in_group;

    StatementFilter() = default;
    StatementFilter(size_t len) {
      seconded_in_group.bits.assign(len, false);
      validated_in_group.bits.assign(len, false);
    }
  };

  struct BackedCandidateManifest {
    SCALE_TIE(6);
    RelayHash relay_parent;
    CandidateHash candidate_hash;
    GroupIndex group_index;
    ParachainId para_id;
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

  struct BackedCandidateAcknowledgement {
    SCALE_TIE(2);
    CandidateHash candidate_hash;
    StatementFilter statement_knowledge;
  };

  using StatementDistributionMessage =
      boost::variant<StatementDistributionMessageStatement,  // 0
                     BackedCandidateManifest,                // 1
                     BackedCandidateAcknowledgement          // 2
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
