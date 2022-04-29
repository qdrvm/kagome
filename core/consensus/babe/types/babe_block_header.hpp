/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_BLOCK_HEADER_HPP
#define KAGOME_BABE_BLOCK_HEADER_HPP

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/slot.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::consensus {
  /**
   * Contains specific data, needed in BABE for validation
   *
   * @see
   * https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/consensus/babe/src/digests.rs#L74
   */
  struct BabeBlockHeader {
    SlotType slot_assignment_type{};

    /// slot, in which the block was produced
    BabeSlotNumber slot_number;

    /// output of VRF function
    crypto::VRFOutput vrf_output{};

    /// authority index of the producer
    primitives::AuthorityIndex authority_index;

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

    bool needAuthorCheck() const {
      return slot_assignment_type == SlotType::SecondaryPlain
             or slot_assignment_type == SlotType::SecondaryVRF;
    }
  };

  /**
   * @brief outputs object of type BabeBlockHeader to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BabeBlockHeader &bh) {
    s << bh.slot_assignment_type;
    s << bh.authority_index << bh.slot_number;
    if (bh.needVRFCheck()) {
      s << bh.vrf_output;
    }
    return s;
  }

  /**
   * @brief decodes object of type BabeBlockHeader from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BabeBlockHeader &bh) {
    s >> bh.slot_assignment_type;
    s >> bh.authority_index >> bh.slot_number;
    if (bh.needVRFCheck()) {
      s >> bh.vrf_output;
    }
    return s;
  }
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_BLOCK_HEADER_HPP
