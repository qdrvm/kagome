/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "outcome/outcome.hpp"
#include <fmt/format.h>

#include "network/types/collator_messages.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/types.hpp"

namespace kagome::parachain::fragment {

  template <typename... Args>
  using HashMap = std::unordered_map<Args...>;
  template <typename... Args>
  using HashSet = std::unordered_set<Args...>;
  template <typename... Args>
  using Vec = std::vector<Args...>;
  using BitVec = scale::BitVec;
  using ParaId = ParachainId;
  template <typename... Args>
  using Option = std::optional<Args...>;
  template <typename... Args>
  using Map = std::map<Args...>;
  template <typename... Args>
  using Ref = std::reference_wrapper<Args...>;

  using NodePointerRoot = network::Empty;
  using NodePointerStorage = size_t;
  using NodePointer = boost::variant<NodePointerRoot, NodePointerStorage>;

  using CandidateCommitments = network::CandidateCommitments;
  using PersistedValidationData = runtime::PersistedValidationData;

  /// Indicates the relay-parents whose fragment chain a candidate
  /// is present in or can be added in (right now or in the future).
  using HypotheticalMembership = Vec<Hash>;

  /// A collection of ancestor candidates of a parachain.
  using Ancestors = HashSet<CandidateHash>;

  struct RelayChainBlockInfo {
    /// The hash of the relay-chain block.
    Hash hash;
    /// The number of the relay-chain block.
    BlockNumber number;
    /// The storage-root of the relay-chain block.
    Hash storage_root;
  };

  /// Information about a relay-chain block, to be used when calling this module
  /// from prospective parachains.
  struct BlockInfoProspectiveParachains {
    /// The hash of the relay-chain block.
    Hash hash;
    /// The hash of the parent relay-chain block.
    Hash parent_hash;
    /// The number of the relay-chain block.
    BlockNumber number;
    /// The storage-root of the relay-chain block.
    Hash storage_root;

    RelayChainBlockInfo as_relay_chain_block_info() const {
      return RelayChainBlockInfo{
          .hash = hash,
          .number = number,
          .storage_root = storage_root,
      };
    }
  };

  struct ProspectiveCandidate {
    /// The commitments to the output of the execution.
    network::CandidateCommitments commitments;
    /// The persisted validation data used to create the candidate.
    runtime::PersistedValidationData persisted_validation_data;
    /// The hash of the PoV.
    Hash pov_hash;
    /// The validation code hash used by the candidate.
    ValidationCodeHash validation_code_hash;

    ProspectiveCandidate(network::CandidateCommitments c,
                         runtime::PersistedValidationData p,
                         Hash h,
                         ValidationCodeHash v)
        : commitments{std::move(c)},
          persisted_validation_data{std::move(p)},
          pov_hash{std::move(h)},
          validation_code_hash{std::move(v)} {}
  };

  template <typename T>
  concept HypotheticalOrConcreteCandidate = requires(T v) {
    v.get_commitments();
    v.get_persisted_validation_data();
    v.get_validation_code_hash();
    v.get_parent_head_data_hash();
    v.get_output_head_data_hash();
    v.get_relay_parent();
    v.get_candidate_hash();
  };

}  // namespace kagome::parachain::fragment
