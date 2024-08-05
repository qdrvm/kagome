/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <scale/bitvec.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_types.hpp"
#include "parachain/approval/approval.hpp"
#include "parachain/types.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  struct AvailableData;
}  // namespace kagome::runtime

namespace kagome::network {
  using namespace parachain;

  /// NU element.
  using Dummy = std::tuple<>;

  /// Empty element.
  using Empty = std::tuple<>;

  /**
   * Collator -> Validator message.
   * Advertisement of a collation.
   */
  struct CollatorAdvertisement {
    SCALE_TIE(1);

    primitives::BlockHash relay_parent;  /// Hash of the parachain block.
  };

  /**
   * Collator -> Validator message.
   * Declaration of the intent to advertise a collation.
   */
  struct CollatorDeclaration {
    SCALE_TIE(3);
    CollatorPublicKey collator_id;  /// Public key of the collator.
    ParachainId para_id;            /// Parachain Id.
    Signature signature;  /// Signature of the collator using the PeerId of the
    /// collators node.
  };

  /// A chunk of erasure-encoded block data.
  struct ErasureChunk {
    SCALE_TIE(3);

    /// The erasure-encoded chunk of data belonging to the candidate block.
    common::Buffer chunk;
    /// The index of this erasure-encoded chunk of data.
    ChunkIndex index;
    /// Proof for this chunk's branch in the Merkle tree.
    ChunkProof proof;
  };

  /**
   * PoV
   */
  struct ParachainBlock {
    SCALE_TIE(1);

    /// Contains the necessary data to for parachain specific state transition
    /// logic
    common::Buffer payload;
  };

  using PoV = ParachainBlock;
  using RequestPov = CandidateHash;
  using ResponsePov = boost::variant<ParachainBlock, Empty>;

  /**
   * Contains information about the candidate and a proof of the results of its
   * execution.
   */
  struct CandidateReceipt {
    CandidateDescriptor descriptor;  /// Candidate descriptor
    Hash commitments_hash;           /// Hash of candidate commitments

    const Hash &hash(const crypto::Hasher &hasher) const {
      if (not hash_.has_value()) {
        hash_.emplace(hasher.blake2b_256(
            ::scale::encode(std::tie(descriptor, commitments_hash)).value()));
      }
      return hash_.value();
    }

    inline bool operator==(const CandidateReceipt &other) const {
      return descriptor == other.descriptor
         and commitments_hash == other.commitments_hash;
    }

    friend inline scale::ScaleDecoderStream &operator>>(
        scale::ScaleDecoderStream &s, CandidateReceipt &cr) {
      return s >> cr.descriptor >> cr.commitments_hash;
    }

    friend inline scale::ScaleEncoderStream &operator<<(
        scale::ScaleEncoderStream &s, const CandidateReceipt &cr) {
      return s << cr.descriptor << cr.commitments_hash;
    }

   private:
    mutable std::optional<Hash> hash_{};
  };

  struct CollationResponse {
    SCALE_TIE(2);

    /// Candidate receipt
    CandidateReceipt receipt;

    /// PoV block
    ParachainBlock pov;
  };

  struct CollationWithParentHeadData {
    SCALE_TIE(3);
    /// The receipt of the candidate.
    CandidateReceipt receipt;

    /// Candidate's proof of validity.
    ParachainBlock pov;

    /// The head data of the candidate's parent.
    /// This is needed for elastic scaling to work.
    HeadData parent_head_data;
  };

  using ReqCollationResponseData =
      boost::variant<CollationResponse, CollationWithParentHeadData>;

  /**
   * Sent by clients who want to retrieve the advertised collation at the
   * specified relay chain block.
   */
  struct CollationFetchingRequest {
    SCALE_TIE(2);

    Hash relay_parent;    /// Hash of the relay chain block
    ParachainId para_id;  /// Parachain Id.
  };

  /**
   * Sent by nodes to the clients who issued a collation fetching request.
   */
  struct CollationFetchingResponse {
    SCALE_TIE(1);

    ReqCollationResponseData response_data;  /// Response data
  };

  /**
   * Sent by clients who want to retrieve chunks of a parachain candidate.
   */
  struct FetchChunkRequest {
    SCALE_TIE(2);

    CandidateHash candidate;  /// parachain candidate hash
    ChunkIndex chunk_index;   /// index of the chunk
  };

  /**
   * Sent by nodes to the clients who issued a chunk fetching request.
   * Version 1 (obsolete)
   */
  struct ChunkObsolete {
    SCALE_TIE(2);

    /// chunk data
    common::Buffer data;
    /// chunk proof
    ChunkProof proof;
  };
  using FetchChunkResponseObsolete = boost::variant<ChunkObsolete, Empty>;

  /**
   * Sent by nodes to the clients who issued a chunk fetching request. Version 2
   */
  struct Chunk {
    SCALE_TIE(3);

    /// chunk data
    common::Buffer data;
    /// chunk index
    ChunkIndex chunk_index;
    /// chunk proof
    ChunkProof proof;
  };
  using FetchChunkResponse = boost::variant<Chunk, Empty>;

  using FetchAvailableDataRequest = CandidateHash;
  using FetchAvailableDataResponse =
      boost::variant<runtime::AvailableData, Empty>;

  struct CommittedCandidateReceipt {
    SCALE_TIE(2);

    CandidateDescriptor descriptor;    /// Candidate descriptor
    CandidateCommitments commitments;  /// commitments retrieved from validation
    /// result and produced by the execution
    /// and validation parachain candidate

    CandidateReceipt to_plain(const crypto::Hasher &hasher) const {
      CandidateReceipt receipt;
      receipt.descriptor = descriptor,
      receipt.commitments_hash =
          hasher.blake2b_256(scale::encode(commitments).value());
      return receipt;
    }
  };

  struct FetchStatementRequest {
    SCALE_TIE(2);
    RelayHash relay_parent;
    CandidateHash candidate_hash;
  };
  using FetchStatementResponse = boost::variant<CommittedCandidateReceipt>;

  struct ValidityAttestation {
    SCALE_TIE(2);

    struct Implicit : Empty {};
    struct Explicit : Empty {};

    boost::variant<Dummy, Implicit, Explicit> kind;
    Signature signature;
  };

  struct BackedCandidate {
    SCALE_TIE(3);

    CommittedCandidateReceipt candidate;
    std::vector<ValidityAttestation> validity_votes;
    scale::BitVec validator_indices;

    /// Creates `BackedCandidate` from args.
    static BackedCandidate from(
        CommittedCandidateReceipt candidate_,
        std::vector<ValidityAttestation> validity_votes_,
        scale::BitVec validator_indices_,
        std::optional<CoreIndex> core_index_) {
      BackedCandidate backed{
          .candidate = std::move(candidate_),
          .validity_votes = std::move(validity_votes_),
          .validator_indices = std::move(validator_indices_),
      };

      if (core_index_) {
        backed.inject_core_index(*core_index_);
      }

      return backed;
    }

    void inject_core_index(CoreIndex core_index) {
      scale::BitVec core_index_to_inject;
      core_index_to_inject.bits.assign(8, false);

      auto val = uint8_t(core_index);
      for (size_t i = 0; i < 8; ++i) {
        core_index_to_inject.bits[i] = (val >> i) & 1;
      }
      validator_indices.bits.insert(validator_indices.bits.end(),
                                    core_index_to_inject.bits.begin(),
                                    core_index_to_inject.bits.end());
    }
  };

  using CandidateState =
      boost::variant<Unused<0>,                  /// not used
                     CommittedCandidateReceipt,  /// Candidate receipt. Should
                                                 /// be sent if validator
                                                 /// seconded the candidate
                     CandidateHash  /// validator has deemed the candidate valid
                                    /// and send the candidate hash
                     >;

  struct Statement {
    SCALE_TIE(1);
    Statement() = default;
    Statement(CandidateState &&c) : candidate_state{std::move(c)} {}
    CandidateState candidate_state{Unused<0>{}};
  };
  using SignedStatement = IndexedAndSigned<Statement>;

  inline std::ostream &operator<<(std::ostream &os, const SignedStatement &t) {
    return os << "Statement (validator index:" << t.payload.ix << ')';
  }

  struct Seconded {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// relay parent hash
    SignedStatement statement{};         /// statement of seconded candidate
  };

  /// Signed availability bitfield.
  using SignedBitfield = parachain::IndexedAndSigned<scale::BitVec>;

  struct BitfieldDistribution {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// Hash of the relay chain block
    SignedBitfield data;
  };

  /// Data that makes a statement unique.
  struct StatementMetadata {
    SCALE_TIE(2);

    /// Relay parent this statement is relevant under.
    primitives::BlockHash relay_parent;
    /// Hash of candidate that was used create the `CommitedCandidateRecept`.
    primitives::BlockHash candidate_hash;
  };

  /// A succinct representation of a peer's view. This consists of a bounded
  /// amount of chain heads and the highest known finalized block number.
  ///
  /// Up to `N` (5?) chain heads.
  /// The rust representation:
  /// https://github.com/paritytech/polkadot/blob/master/node/network/protocol/src/lib.rs#L160
  struct View {
    SCALE_TIE(2);

    /// A bounded amount of chain heads.
    /// Invariant: Sorted.
    std::vector<primitives::BlockHash> heads_;

    /// The highest known finalized block number.
    primitives::BlockNumber finalized_number_;

    bool contains(const primitives::BlockHash &hash) const {
      const auto it = std::lower_bound(heads_.begin(), heads_.end(), hash);
      return it != heads_.end() && *it == hash;
    }
  };

  using LargeStatement = parachain::IndexedAndSigned<StatementMetadata>;
  using StatementDistributionMessage = boost::variant<Seconded, LargeStatement>;

  /**
   * Collator -> Validator and Validator -> Collator if seconded message.
   * Type of the appropriate message.
   */
  using CollationMessage = boost::variant<
      CollatorDeclaration,    /// collator -> validator. Declare collator.
      CollatorAdvertisement,  /// collator -> validator. Make advertisement of
      /// the collation
      Dummy,    /// not used
      Dummy,    /// not used
      Seconded  /// validator -> collator. Candidate was seconded.
      >;

  /**
   * Indicates the availability vote of a validator for a given candidate.
   */
  using BitfieldDistributionMessage = boost::variant<BitfieldDistribution>;

  using IndirectSignedApprovalVote =
      parachain::approval::IndirectSignedApprovalVote;

  struct Assignment {
    SCALE_TIE(2);

    kagome::parachain::approval::IndirectAssignmentCert
        indirect_assignment_cert;
    CandidateIndex candidate_ix;
  };

  struct Assignments {
    SCALE_TIE(1);

    std::vector<Assignment> assignments;  /// Assignments for candidates in
                                          /// recent, unfinalized blocks.
  };

  struct Approvals {
    SCALE_TIE(1);

    std::vector<IndirectSignedApprovalVote>
        approvals;  /// Approvals for candidates in some recent, unfinalized
                    /// block.
  };

  /// Network messages used by the approval distribution subsystem.
  using ApprovalDistributionMessage = boost::variant<
      /// Assignments for candidates in recent, unfinalized blocks.
      ///
      /// Actually checking the assignment may yield a different result.
      Assignments,
      /// Approvals for candidates in some recent, unfinalized block.
      Approvals>;

  /// Attestation is either an implicit or explicit attestation of the validity
  /// of a parachain candidate, where 1 implies an implicit vote (in
  /// correspondence of a Seconded statement) and 2 implies an explicit
  /// attestation (in correspondence of a Valid statement). Both variants are
  /// followed by the signature of the validator.
  using Attestation = boost::variant<Unused<0>,                            // 0
                                     Tagged<Signature, struct Implicit>,   // 1
                                     Tagged<Signature, struct Explicit>>;  // 2

  struct CommittedCandidate {
    SCALE_TIE(3);

    CommittedCandidateReceipt
        candidate_receipt;  /// Committed candidate receipt
    std::vector<Attestation>
        validity_votes;  /// An array of validity votes themselves, expressed as
                         /// signatures
    std::vector<bool> indices;  /// A bitfield of indices of the validators
                                /// within the validator group
  };

  //  using DisputeStatement =
  //      boost::variant<Tagged<Empty, struct ExplicitStatement>,
  //                     Tagged<common::Hash256, struct SecondedStatement>,
  //                     Tagged<common::Hash256, struct ValidStatement>,
  //                     Tagged<Empty, struct AprovalVote>>;
  //
  //  struct Vote {
  //    SCALE_TIE(3);
  //
  //    uint32_t validator_index;  /// An unsigned 32-bit integer indicating the
  //                               /// validator index in the authority set
  //    Signature signature;       /// The signature of the validator
  //    DisputeStatement
  //        statement;  /// A varying datatype and implies the dispute statement
  //  };

  /**
   * Validator -> Validator.
   * Used by validators to broadcast relevant information about certain steps in
   * the A&V process.
   */
  using ValidatorProtocolMessage = boost::variant<
      Dummy,                         /// NU
      BitfieldDistributionMessage,   /// bitfield distribution message
      Dummy,                         /// NU
      StatementDistributionMessage,  /// statement distribution message
      ApprovalDistributionMessage    /// approval distribution message
      >;
  using CollationProtocolMessage = boost::variant<CollationMessage>;

  template <typename T, typename... AllowedTypes>
  struct AllowerTypeChecker {
    static constexpr bool allowed = (std::is_same_v<T, AllowedTypes> || ...);
  };

  /// Proposal of a parachain candidate.
  using CompactStatementSeconded =
      Tagged<primitives::BlockHash, struct SecondedTag>;

  /// State that a parachain
  /// candidate is valid.
  using CompactStatementValid = Tagged<primitives::BlockHash, struct ValidTag>;

  /// Statements that can be made about parachain candidates. These are the
  /// actual values that are signed.
  using CompactStatement =
      std::variant<CompactStatementSeconded, CompactStatementValid>;

  /// ViewUpdate message. Maybe will be implemented later.
  struct ViewUpdate {
    SCALE_TIE(1)
    View view;
  };

  /// Information about a core which is currently occupied.
  struct ScheduledCore {
    SCALE_TIE(2)

    /// The ID of a para scheduled.
    ParachainId para_id;
    /// The collator required to author the block, if any.
    std::optional<CollatorId> collator;
  };

  inline const CandidateHash &candidateHash(const CompactStatement &val) {
    auto p = visit_in_place(
        val, [&](const auto &v) -> std::reference_wrapper<const CandidateHash> {
          return v;
        });
    return p.get();
  }

  inline CandidateHash candidateHash(const crypto::Hasher &hasher,
                                     const CommittedCandidateReceipt &receipt) {
    auto commitments_hash =
        hasher.blake2b_256(scale::encode(receipt.commitments).value());
    return hasher.blake2b_256(
        ::scale::encode(std::tie(receipt.descriptor, commitments_hash))
            .value());
  }

  inline CandidateHash candidateHash(const crypto::Hasher &hasher,
                                     const CandidateState &statement) {
    if (auto receipt = boost::get<CommittedCandidateReceipt>(&statement)) {
      return candidateHash(hasher, *receipt);
    }
    return boost::get<CandidateHash>(statement);
  }
}  // namespace kagome::network

template <>
struct fmt::formatter<kagome::network::SignedBitfield> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const kagome::network::SignedBitfield &val,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    char buf[8] = {0};
    const auto &bits = val.payload.payload.bits;

    static_assert(sizeof(buf) > 1, "Because of last zero-terminate symbol");
    size_t ix = 0;
    for (; ix < std::min(bits.size(), sizeof(buf) - size_t(1)); ++ix) {
      buf[ix] = bits[ix] ? '1' : '0';
    }

    return fmt::format_to(ctx.out(),
                          "sig={}, validator={}, bits=[0b{}{}]",
                          val.signature,
                          val.payload.ix,
                          buf,
                          ix == bits.size() ? "" : "…");
  }
};
