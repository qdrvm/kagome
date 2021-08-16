/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_BLOCK_HEADER_HPP
#define KAGOME_BABE_BLOCK_HEADER_HPP

#include "consensus/babe/common.hpp"
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
    static constexpr uint8_t kVRFHeader = 0x01;
    static constexpr uint8_t kSecondaryHeaderCheck = 0x02;

    uint8_t check_type{};
    /// slot, in which the block was produced
    BabeSlotNumber slot_number{};
    /// output of VRF function
    crypto::VRFOutput vrf_output;
    /// authority index of the producer
    primitives::AuthorityIndex authority_index{};

    bool needVRFCheck() const {
      return (check_type & kVRFHeader) != 0;
    }

    bool needVRFWithThresholdCheck() const {
      return (check_type & (kVRFHeader | kSecondaryHeaderCheck)) == kVRFHeader;
    }

    bool needAuthorCheck() const {
      return (check_type & kSecondaryHeaderCheck) != 0;
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
    return s << bh.check_type << bh.authority_index << bh.slot_number
             << bh.vrf_output;
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
    s >> bh.check_type >> bh.authority_index >> bh.slot_number;
    if (bh.needVRFCheck()) s >> bh.vrf_output;
    return s;
  }
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_BLOCK_HEADER_HPP
