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

  struct CollatorProtocolMessageAdvertiseCollation {
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
    CandidateHash hash;
  };

  struct ValidCandidateHash {
    CandidateHash hash;
  };

  using CompactStatement =
      boost::variant<Empty, SecondedCandidateHash, ValidCandidateHash>;

  struct StatementDistributionMessageStatement {
    RelayHash relay_parent;
    IndexedAndSigned<CompactStatement> compact;
  };

  using v1StatementDistributionMessage = network::StatementDistributionMessage;

  struct StatementFilter {
    /// Seconded statements. '1' is known or undesired.
    scale::BitVec seconded_in_group;
    /// Valid statements. '1' is known or undesired.
    scale::BitVec validated_in_group;
  };

  struct BackedCandidateManifest {
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
    CandidateHash candidate_hash;
    StatementFilter statement_knowledge;
  };

  struct StatementDistributionMessage {
    uint8_t index;
    union {
      StatementDistributionMessageStatement statement;   // 0
      BackedCandidateManifest manifest;                  // 1
      BackedCandidateAcknowledgement acknowledgement;    // 2
      v1StatementDistributionMessage v1_compartibility;  // 255
    } data;
  };

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

  struct CandidatePendingAvailability {
    SCALE_TIE(5);
    /// The hash of the candidate.
    CandidateHash candidate_hash;
    /// The candidate's descriptor.
    CandidateDescriptor descriptor;
    /// The commitments of the candidate.
    CandidateCommitments commitments;
    /// The candidate's relay parent's number.
    BlockNumber relay_parent_number;
    /// The maximum Proof-of-Validity size allowed, in bytes.
    uint32_t max_pov_size;
  };

}  // namespace kagome::network::vstaging

namespace kagome::network {

  template <typename V1, typename VStaging>
  using Versioned = boost::variant<V1, VStaging>;

  using VersionedCollatorProtocolMessage =
      Versioned<kagome::network::CollationMessage,
                kagome::network::vstaging::CollatorProtocolMessage>;
  using VersionedStatementDistributionMessage =
      Versioned<kagome::network::StatementDistributionMessage,
                kagome::network::vstaging::StatementDistributionMessage>;
}  // namespace kagome::network

#endif  // KAGOME_COLLATOR_MESSAGES_VSTAGING_HPP
