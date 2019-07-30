/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_BLOCK_HEADER_HPP
#define KAGOME_BABE_BLOCK_HEADER_HPP

#include "consensus/babe/common.hpp"
#include "crypto/vrf_types.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::consensus {
  /**
   * Contains specific data, needed in BABE for validation
   */
  struct BabeBlockHeader {
    /// slot, in which the block was produced
    BabeSlotNumber slot_number;
    /// proof that producer is a slot leader
    crypto::VRFValue vrf_proof;
    /// output of VRF function
    crypto::VRFOutput vrf_output;
    /// authority index of the producer
    primitives::AuthorityIndex authority_index;
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
    // TODO(akvinikym): order of encoding/decoding may be incorrect
    return s << bh.slot_number << bh.vrf_proof << bh.vrf_output
             << bh.authority_index;
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
    return s >> bh.slot_number >> bh.vrf_proof >> bh.vrf_output
           >> bh.authority_index;
  }
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_BLOCK_HEADER_HPP
