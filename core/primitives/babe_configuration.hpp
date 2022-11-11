/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP
#define KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP

#include <fmt/core.h>

#include "common/blob.hpp"
#include "consensus/babe/common.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/authority.hpp"

namespace kagome::primitives {

  using BabeSlotNumber = uint64_t;
  using BabeClock = clock::SystemClock;
  using BabeDuration = BabeClock::Duration;
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

  enum class AllowedSlots : uint8_t {
    PrimaryOnly,
    PrimaryAndSecondaryPlain,
    PrimaryAndSecondaryVRF
  };

  inline std::string_view to_string(AllowedSlots s) {
    switch (s) {
      case AllowedSlots::PrimaryOnly:
        return "Primary only";
      case AllowedSlots::PrimaryAndSecondaryPlain:
        return "Primary and Secondary Plain";
      case AllowedSlots::PrimaryAndSecondaryVRF:
        return "Primary and Secondary VRF";
    }
    return "Unknown";
  }

  /// Configuration data used by the BABE consensus engine.
  struct BabeConfiguration {
    /// The slot duration in milliseconds for BABE. Currently, only
    /// the value provided by this type at genesis will be used.
    ///
    /// Dynamic slot duration may be supported in the future.
    BabeDuration slot_duration{};  // must be permanent

    BabeSlotNumber epoch_length{};  // must be permanent

    /// A constant value that is used in the threshold calculation formula.
    /// Expressed as a rational where the first member of the tuple is the
    /// numerator and the second is the denominator. The rational should
    /// represent a value between 0 and 1.
    /// In substrate it is called `c`
    /// In the threshold formula calculation, `1 - leadership_rate` represents
    /// the probability of a slot being empty.
    std::pair<uint64_t, uint64_t> leadership_rate;  // changes by NextConfigData

    /// The authorities for block production
    AuthorityList authorities;  // can be changed by NextEpochData & OnDisabled

    /// The randomness for the genesis epoch.
    Randomness randomness;  // can be changed by NextEpochData

    /// Type of allowed slots.
    AllowedSlots allowed_slots;  // can be changed by NextConfigData

    bool isSecondarySlotsAllowed() const {
      return allowed_slots == primitives::AllowedSlots::PrimaryAndSecondaryPlain
             or allowed_slots
                    == primitives::AllowedSlots::PrimaryAndSecondaryVRF;
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BabeConfiguration &config) {
    size_t slot_duration_u64 =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            config.slot_duration)
            .count();
    return s << slot_duration_u64 << config.epoch_length
             << config.leadership_rate << config.authorities
             << config.randomness << static_cast<uint8_t>(config.allowed_slots);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BabeConfiguration &config) {
    size_t slot_duration_u64{};
    uint8_t allowed_slots;
    s >> slot_duration_u64 >> config.epoch_length >> config.leadership_rate
        >> config.authorities >> config.randomness >> allowed_slots;
    config.slot_duration = std::chrono::milliseconds(slot_duration_u64);
    config.allowed_slots = static_cast<AllowedSlots>(allowed_slots);
    return s;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BABE_CONFIGURATION_HPP
