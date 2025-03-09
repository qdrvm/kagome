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
     * @param encoder stream reference
     * @param v value to output
     * @return reference to stream
     */
    friend inline void encode(const BabeBlockHeader &bh,
                              scale::Encoder &encoder) {
      encode(
          std::tie(bh.slot_assignment_type, bh.authority_index, bh.slot_number),
          encoder);
      if (bh.needVRFCheck()) {
        encode(bh.vrf_output, encoder);
      }
    }

    /**
     * @brief decodes object of type BabeBlockHeader from stream
     * @param decoder stream reference
     * @param v value to output
     * @return reference to stream
     */
    friend inline void decode(BabeBlockHeader &bh, scale::Decoder &decoder) {
      decode(
          std::tie(bh.slot_assignment_type, bh.authority_index, bh.slot_number),
          decoder);
      if (bh.needVRFCheck()) {
        decode(bh.vrf_output, decoder);
      }
    }
  };
}  // namespace kagome::consensus::babe
