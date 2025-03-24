/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/bit_vector.hpp>

#include "primitives/transcript.hpp"
#include "parachain/types.hpp"

// Polkadot-sdk implementation:
// https://github.com/paritytech/polkadot-sdk/blob/6b854acc69cd64f7c0e6cdb606e741e630e45032/polkadot/node/core/approval-voting/src/criteria.rs#L186
kagome::primitives::Transcript assigned_core_transcript(
    const kagome::parachain::CoreIndex &core_index) {
  kagome::primitives::Transcript t;
  t.initialize({'A', '&', 'V', ' ', 'A', 'S', 'S', 'I', 'G', 'N', 'E', 'D'});

  t.append_message({'c', 'o', 'r', 'e'}, core_index);

  return {t};
}

// Polkadot-sdk implementation:
// https://github.com/paritytech/polkadot-sdk/blob/6b854acc69cd64f7c0e6cdb606e741e630e45032/polkadot/node/core/approval-voting/src/criteria.rs#L395
kagome::primitives::Transcript assigned_cores_transcript(
    const kagome::scale::BitVector &core_indices) {
  kagome::primitives::Transcript t;
  t.initialize({'A',
                '&',
                'V',
                ' ',
                'A',
                'S',
                'S',
                'I',
                'G',
                'N',
                'E',
                'D',
                ' ',
                'v',
                '2'});
  t.append_message({'c', 'o', 'r', 'e', 's'},
                   kagome::scale::encode(core_indices).value());
  return {t};
}

// Polkadot-sdk implementation:
// https://github.com/paritytech/polkadot-sdk/blob/6b854acc69cd64f7c0e6cdb606e741e630e45032/polkadot/node/core/approval-voting/src/criteria.rs#L82
kagome::primitives::Transcript relay_vrf_modulo_transcript_v1(
    const RelayVRFStory &relay_vrf_story, uint32_t sample) {
  kagome::primitives::Transcript transcript;
  transcript.initialize({'A', '&', 'V', ' ', 'M', 'O', 'D'});
  transcript.append_message({'R', 'C', '-', 'V', 'R', 'F'},
                            relay_vrf_story.data);
  transcript.append_message({'s', 'a', 'm', 'p', 'l', 'e'}, sample);
  return {transcript};
}

// Polkadot-sdk implementation:
// https://github.com/paritytech/polkadot-sdk/blob/6b854acc69cd64f7c0e6cdb606e741e630e45032/polkadot/node/core/approval-voting/src/criteria.rs#L90
kagome::primitives::Transcript relay_vrf_modulo_transcript_v2(
    const RelayVRFStory &relay_vrf_story) {
  kagome::primitives::Transcript transcript;
  transcript.initialize({'A', '&', 'V', ' ', 'M', 'O', 'D', ' ', 'v', '2'});
  transcript.append_message({'R', 'C', '-', 'V', 'R', 'F'},
                            relay_vrf_story.data);
  return {transcript};
}
