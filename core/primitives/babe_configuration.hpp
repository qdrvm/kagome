/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP
#define KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP

#include "common/blob.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/authority.hpp"

namespace kagome::primitives {

  using BabeSlotNumber = uint64_t;
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

  /// Configuration data used by the BABE consensus engine.
  struct BabeConfiguration {
    /// The slot duration in milliseconds for BABE. Currently, only
    /// the value provided by this type at genesis will be used.
    ///
    /// Dynamic slot duration may be supported in the future.
    uint64_t slot_duration;

    BabeSlotNumber epoch_length;

    /// A constant value that is used in the threshold calculation formula.
    /// Expressed as a rational where the first member of the tuple is the
    /// numerator and the second is the denominator. The rational should
    /// represent a value between 0 and 1.
    /// In the threshold formula calculation, `1 - c` represents the probability
    /// of a slot being empty.
    std::pair<uint64_t, uint64_t> c;

    /// The authorities for the genesis epoch.
    std::vector<Authority> genesis_authorities;

    /// The randomness for the genesis epoch.
    Randomness randomness;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BabeConfiguration &config) {
    return s << config.slot_duration << config.epoch_length << config.c
             << config.genesis_authorities << config.randomness;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BabeConfiguration &config) {
    return s >> config.slot_duration >> config.epoch_length >> config.c
           >> config.genesis_authorities >> config.randomness;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP
