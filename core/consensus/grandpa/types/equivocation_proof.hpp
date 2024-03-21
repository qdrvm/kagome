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

  /// Wrapper object for GRANDPA equivocation proofs, useful for unifying
  /// prevote and precommit equivocations under a common type.
  // https://github.com/paritytech/polkadot-sdk/blob/0e49ed72aa365475e30069a5c30e251a009fdacf/substrate/primitives/consensus/grandpa/src/lib.rs#L272
  struct Equivocation {
    /// Round stage: prevote or precommit
    const VoteType stage;
    /// The round number equivocated in.
    const RoundNumber round_number;
    /// The first vote in the equivocation.
    const SignedMessage first;
    /// The second vote in the equivocation.
    const SignedMessage second;

    Equivocation(RoundNumber round_number,
                 const SignedMessage &first,
                 const SignedMessage &second)
        : stage{first.is<Prevote>() ? VoteType::Prevote : VoteType::Precommit},
          round_number(round_number),
          first(first),
          second(second) {
      BOOST_ASSERT((first.is<Prevote>() and second.is<Prevote>())
                   or (first.is<Precommit>() and second.is<Precommit>()));
      BOOST_ASSERT(first.id == second.id);
    };

    AuthorityId offender() const {
      return first.id;
    }

    RoundNumber round() const {
      return round_number;
    }

    friend scale::ScaleEncoderStream &operator<<(
        scale::ScaleEncoderStream &s, const Equivocation &equivocation) {
      return s << equivocation.stage << equivocation.round_number
               << equivocation.first.id << equivocation.first
               << equivocation.second;
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
