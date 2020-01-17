/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_BLOCK_HEADER_HPP
#define KAGOME_BABE_BLOCK_HEADER_HPP

#include "consensus/babe/common.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/authority.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::consensus {
  /**
   * Contains specific data, needed in BABE for validation
   */
  struct BabeBlockHeader {
    /// slot, in which the block was produced
    BabeSlotNumber slot_number{};
    /// output of VRF function
    crypto::VRFOutput vrf_output;
    /// authority index of the producer
    primitives::AuthorityIndex authority_index{};
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
    uint8_t fake_type_index = 0;
    uint32_t fake_weight = 1;
    return s << fake_type_index << bh.authority_index << bh.slot_number
             << fake_weight << bh.vrf_output;
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
    uint8_t fake_type_index = 0;
    uint32_t fake_weight = 0;
    return s >> fake_type_index >> bh.authority_index >> bh.slot_number
           >> fake_weight >> bh.vrf_output;
  }
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_BLOCK_HEADER_HPP
