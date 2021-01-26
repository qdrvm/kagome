/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_EPOCHDESCRIPTOR
#define KAGOME_CONSENSUS_EPOCHDESCRIPTOR

#include "consensus/babe/common.hpp"

namespace kagome::consensus {

  /**
   * Metainformation about the Babe epoch
   */
  struct EpochDescriptor {
    EpochNumber epoch_number = 0;

    /// starting slot of the epoch
    BabeSlotNumber start_slot = 0;

    BabeTimePoint starting_slot_finish_time;

    bool operator==(const EpochDescriptor &other) const {
      return epoch_number == other.epoch_number
             && start_slot == other.start_slot;
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const EpochDescriptor &led) {
    auto starting_slot_finish_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            led.starting_slot_finish_time.time_since_epoch())
            .count();
    return s << led.epoch_number << led.start_slot << starting_slot_finish_time;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, EpochDescriptor &led) {
    int64_t starting_slot_finish_time;
    s >> led.epoch_number >> led.start_slot >> starting_slot_finish_time;
    led.starting_slot_finish_time =
        BabeTimePoint::clock::time_point()
        + std::chrono::milliseconds(starting_slot_finish_time);
    return s;
  }

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_EPOCHDESCRIPTOR
