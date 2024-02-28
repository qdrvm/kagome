/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>

#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/types/authority.hpp"
#include "consensus/grandpa/vote_types.hpp"
#include "primitives/block_header.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::grandpa {

  /// An opaque type used to represent the key ownership proof at the runtime
  /// API boundary. The inner value is an encoded representation of the actual
  /// key ownership proof which will be parameterized when defining the runtime.
  /// At the runtime API boundary this type is unknown and as such we keep this
  /// opaque representation, implementors of the runtime API will have to make
  /// sure that all usages of `OpaqueKeyOwnershipProof` refer to the same type.
  using OpaqueKeyOwnershipProof =
      Tagged<common::Buffer, struct OpaqueKeyOwnershipProofTag>;

  template <VoteType type>
  struct EquivocationT {
    using SignedMessage = std::conditional_t<type == VoteType::Prevote,
                                             SignedPrevote,
                                             SignedPrecommit>;
    SCALE_TIE(4);

    AuthorityId identity;
    RoundNumber round_number;
    SignedMessage first;
    SignedMessage second;
  };

  //  /// Wrapper object for GRANDPA equivocation proofs, useful for unifying
  //  /// prevote and precommit equivocations under a common type.
  //  using Equivocation = boost::variant<
  //      /// Proof of equivocation at prevote stage.
  //      EquivocationT<VoteType::Prevote>,
  //      /// Proof of equivocation at precommit stage.
  //      EquivocationT<VoteType::Precommit>>;

  struct Equivocation {
    // Round stage: prevote or precommit
    VoteType stage;

    RoundNumber round_number;

    SignedMessage first;
    SignedMessage second;

    Equivocation(const SignedMessage &first, const SignedMessage &second)
        : stage{[&]() -> VoteType {
            BOOST_ASSERT((first.is<Prevote>() and second.is<Prevote>())
                         or (first.is<Precommit>() and second.is<Precommit>()));
            return first.is<Prevote>() ? VoteType::Prevote
                                       : VoteType::Precommit;
          }()},
          round_number(),
          first(first),
          second(second){};

    AuthorityId offender() const {
      return first.id;
    }

    RoundNumber round() const {
      return 0;
    }

    friend scale::ScaleEncoderStream &operator<<(
        scale::ScaleEncoderStream &s, const Equivocation &equivocation) {
      return s << equivocation.stage << equivocation.round_number
               << equivocation.first << equivocation.second;
    }
  };

  /// Proof of voter misbehavior on a given set id. Misbehavior/equivocation
  /// in GRANDPA happens when a voter votes on the same round (either at
  /// prevote or precommit stage) for different blocks. Proving is achieved
  /// by collecting the signed messages of conflicting votes.
  struct EquivocationProof {
    SCALE_TIE(2);

    AuthoritySetId set_id;
    Equivocation equivocation;
  };

}  // namespace kagome::consensus::grandpa
