/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/authority.hpp"
#include "primitives/common.hpp"

#include <scale/scale.hpp>

namespace kagome::consensus::sassafras {
  /**
   * Contains specific data, needed in BABE for validation
   *
   * @see
   * https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/consensus/babe/src/digests.rs#L74
   */
  struct SassafrasBlockHeader {
    /// authority index of the producer
    primitives::AuthorityIndex authority_index;

    /// slot, in which the block was produced
    SlotNumber slot_number;

    /// output of VRF function
    crypto::VRFOutput vrf_output{};

    /**
     * @brief outputs object of type BabeBlockHeader to stream
     * @param s stream reference
     * @param v value to output
     * @return reference to stream
     */
    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const SassafrasBlockHeader &bh) {
      return s << bh.authority_index << bh.slot_number << bh.vrf_output;
    }

    /**
     * @brief decodes object of type BabeBlockHeader from stream
     * @param s stream reference
     * @param v value to output
     * @return reference to stream
     */
    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, SassafrasBlockHeader &bh) {
      return s >> bh.authority_index >> bh.slot_number >> bh.vrf_output;
    }
  };
}  // namespace kagome::consensus::sassafras
