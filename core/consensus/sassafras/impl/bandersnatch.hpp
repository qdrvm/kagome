/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ranges>

#include "common/size_limited_containers.hpp"
#include "consensus/sassafras/vrf.hpp"
#include "primitives/transcript.hpp"

namespace kagome::consensus::sassafras::bandersnatch {

  static constexpr size_t kMaxVrfInputOutputCounts = 3;

  template <typename T>
  using VrfIosVec = common::SLVector<T, kMaxVrfInputOutputCounts>;

  struct VrfInput {
    // (pub(super) bandersnatch_vrfs::VrfInput);
  };

  /// Data to be signed via one of the two provided vrf flavors.
  ///
  /// The object contains a transcript and a sequence of [`VrfInput`]s ready to
  /// be signed.
  ///
  /// The `transcript` summarizes a set of messages which are defining a
  /// particular protocol by automating the Fiat-Shamir transform for challenge
  /// generation. A good explaination of the topic can be found in Merlin
  /// [docs](https://merlin.cool/)
  ///
  /// The `inputs` is a sequence of [`VrfInput`]s which, during the signing
  /// procedure, are first transformed to [`VrfOutput`]s. Both inputs and
  /// outputs are then appended to the transcript before signing the Fiat-Shamir
  /// transform result (the challenge).
  ///
  /// In practice, as a user, all these technical details can be easily ignored.
  /// What is important to remember is:
  /// - *Transcript* is an object defining the protocol and used to produce the
  ///   signature. This object doesn't influence the `VrfOutput`s values.
  /// - *Vrf inputs* is some additional data which is used to produce *vrf
  ///   outputs*. This data will contribute to the signature as well.
  struct VrfSignData {
    /// VRF inputs to be signed.
    VrfIosVec<VrfInput> inputs;
    /// Associated protocol transcript.
    primitives::Transcript transcript;

    //    VrfSignData(RangeOfBytes auto transcript_label,
    //                std::ranges::range auto transcript_data,
    //                std::ranges::range auto inputs_data);
  };

}  // namespace kagome::consensus::sassafras::bandersnatch
