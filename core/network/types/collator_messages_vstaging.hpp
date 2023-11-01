/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATOR_MESSAGES_VSTAGING_HPP
#define KAGOME_COLLATOR_MESSAGES_VSTAGING_HPP

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
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::network::vstaging {
  using Dummy = network::Dummy;
  using Empty = network::Empty;

  using CollatorProtocolMessageDeclare = network::CollatorDeclaration;
  using CollatorProtocolMessageCollationSeconded = network::Seconded;
  using BitfieldDistributionMessage = network::BitfieldDistributionMessage;
  using ApprovalDistributionMessage = network::ApprovalDistributionMessage;

  struct CollatorProtocolMessageAdvertiseCollation {
    SCALE_TIE(3);
    /// Hash of the relay parent advertised collation is based on.
    RelayHash relay_parent;
    /// Candidate hash.
    CandidateHash candidate_hash;
    /// Parachain head data hash before candidate execution.
    Hash parent_head_data_hash;
  };

  using CollatorProtocolMessage =
      boost::variant<CollatorProtocolMessageDeclare,
                     CollatorProtocolMessageAdvertiseCollation,
                     Dummy,
                     Dummy,
                     CollatorProtocolMessageCollationSeconded>;

  struct SecondedCandidateHash {
    SCALE_TIE(1);
    CandidateHash hash;
  };

  struct ValidCandidateHash {
    SCALE_TIE(1);
    CandidateHash hash;
  };

  using CompactStatement =
      boost::variant<Empty, SecondedCandidateHash, ValidCandidateHash>;

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

  struct BackedCandidateAcknowledgement {
    SCALE_TIE(2);
    CandidateHash candidate_hash;
    StatementFilter statement_knowledge;
  };

  using StatementDistributionMessage = boost::variant<
      StatementDistributionMessageStatement,   // 0
      BackedCandidateManifest,                  // 1
      BackedCandidateAcknowledgement    // 2
      //v1StatementDistributionMessage  // 255
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
      std::enable_if_t<AllowerTypeChecker<T,
                                          ValidatorProtocolMessage,
                                          CollationProtocolMessage,
                                          vstaging::ValidatorProtocolMessage,
                                          vstaging::CollatorProtocolMessage
                                          >::allowed,
                       T>,  /// protocol message
      ViewUpdate            /// view update message
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

#endif  // KAGOME_COLLATOR_MESSAGES_VSTAGING_HPP
