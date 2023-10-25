/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "primitives/transcript.hpp"

namespace kagome::consensus::sassafras {

  inline primitives::Transcript &prepareTranscript(
      primitives::Transcript &transcript,
      const Randomness &randomness,
      SlotNumber slot_number,
      EpochNumber epoch) {
    transcript.initialize("SASS"_bytes);
    transcript.append_message("slot number"_bytes, slot_number);
    transcript.append_message("current epoch"_bytes, epoch);
    transcript.append_message("chain randomness"_bytes, std::span(randomness));
    return transcript;
  }

}  // namespace kagome::consensus::sassafras
