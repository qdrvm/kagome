/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/types/authority.hpp"
#include "consensus/babe/types/slot_type.hpp"
#include "consensus/timeline/types.hpp"
#include "primitives/common.hpp"

#include <scale/scale.hpp>

namespace kagome::consensus::babe {
  /**
   * Contains specific data, needed in BABE for validation
   *
   * @see
   * https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/consensus/babe/src/digests.rs#L74
   */
  struct BabeBlockHeader {
    SlotType slot_assignment_type{};

    /// authority index of the producer
    AuthorityIndex authority_index{};

    /// slot, in which the block was produced
    SlotNumber slot_number{};

    /// output of VRF function
    crypto::VRFOutput vrf_output{};

    SlotType slotType() const {
      return slot_assignment_type;
    }

    bool needVRFCheck() const {
      return slot_assignment_type == SlotType::Primary
          or slot_assignment_type == SlotType::SecondaryVRF;
    }

    bool needVRFWithThresholdCheck() const {
      return slot_assignment_type == SlotType::Primary;
    }

    bool isProducedInSecondarySlot() const {
      return slot_assignment_type == SlotType::SecondaryPlain
          or slot_assignment_type == SlotType::SecondaryVRF;
    }

    /**
     * @brief outputs object of type BabeBlockHeader to stream
     * @param s stream reference
     * @param v value to output
     * @return reference to stream
     */
    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const BabeBlockHeader &bh) {
      s << bh.slot_assignment_type << bh.authority_index << bh.slot_number;
      if (bh.needVRFCheck()) {
        s << bh.vrf_output;
      }
      return s;
    }

    /**
     * @brief decodes object of type BabeBlockHeader from stream
     * @param s stream reference
     * @param v value to output
     * @return reference to stream
     */
    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, BabeBlockHeader &bh) {
      s >> bh.slot_assignment_type >> bh.authority_index >> bh.slot_number;
      if (bh.needVRFCheck()) {
        s >> bh.vrf_output;
      }
      return s;
    }
  };
}  // namespace kagome::consensus::babe
