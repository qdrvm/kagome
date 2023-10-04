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

  /// TODO(iceseer): do
  /// new SDM format
  using StatementDistributionMessage = network::StatementDistributionMessage;

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
