/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP
#define KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP

#include "common/blob.hpp"
#include "consensus/babe/common.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/authority.hpp"

namespace kagome::primitives {

  using BabeSlotNumber = uint64_t;
  using BabeClock = clock::SystemClock;
  using BabeDuration = BabeClock::Duration;
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

  /// Configuration data used by the BABE consensus engine.
  struct BabeConfiguration {
    /// The slot duration in milliseconds for BABE. Currently, only
    /// the value provided by this type at genesis will be used.
    ///
    /// Dynamic slot duration may be supported in the future.
    BabeDuration slot_duration{};

    BabeSlotNumber epoch_length{};

    /// A constant value that is used in the threshold calculation formula.
    /// Expressed as a rational where the first member of the tuple is the
    /// numerator and the second is the denominator. The rational should
    /// represent a value between 0 and 1.
    /// In substrate it is called `c`
    /// In the threshold formula calculation, `1 - leadership_rate` represents
    /// the probability of a slot being empty.
    std::pair<uint64_t, uint64_t> leadership_rate;

    /// The authorities for the genesis epoch.
    AuthorityList genesis_authorities;

    /// The randomness for the genesis epoch.
    Randomness randomness;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BabeConfiguration &config) {
    size_t slot_duration_u64 =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            config.slot_duration)
            .count();
    return s << slot_duration_u64 << config.epoch_length
             << config.leadership_rate << config.genesis_authorities
             << config.randomness;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BabeConfiguration &config) {
    size_t slot_duration_u64{};
    s >> slot_duration_u64 >> config.epoch_length >> config.leadership_rate
        >> config.genesis_authorities >> config.randomness;
    config.slot_duration = std::chrono::milliseconds(slot_duration_u64);
    return s;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP
