/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_PREPARETRANSACRIPT
#define KAGOME_CONSENSUS_PREPARETRANSACRIPT

#include "consensus/timeline/types.hpp"
#include "primitives/transcript.hpp"

namespace kagome::consensus::babe {

  inline primitives::Transcript &prepareTranscript(
      primitives::Transcript &transcript,
      const Randomness &randomness,
      SlotNumber slot_number,
      EpochNumber epoch) {
    transcript.initialize({'B', 'A', 'B', 'E'});
    transcript.append_message(
        {'s', 'l', 'o', 't', ' ', 'n', 'u', 'm', 'b', 'e', 'r'}, slot_number);
    transcript.append_message(
        {'c', 'u', 'r', 'r', 'e', 'n', 't', ' ', 'e', 'p', 'o', 'c', 'h'},
        epoch);
    transcript.append_message({'c',
                               'h',
                               'a',
                               'i',
                               'n',
                               ' ',
                               'r',
                               'a',
                               'n',
                               'd',
                               'o',
                               'm',
                               'n',
                               'e',
                               's',
                               's'},
                              randomness.internal_array_reference());
    return transcript;
  }

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_PREPARETRANSACRIPT
