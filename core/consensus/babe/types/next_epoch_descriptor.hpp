/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_TYPES_NEXT_EPOCH_DESCRIPTOR_HPP
#define KAGOME_CORE_CONSENSUS_BABE_TYPES_NEXT_EPOCH_DESCRIPTOR_HPP

#include "primitives/authority.hpp"

namespace kagome::consensus {

  /// Information about the epoch after next epoch
  struct NextEpochDescriptor {
    /// The authorities.
    std::vector<primitives::Authority> authorities;

    /// The value of randomness to use for the slot-assignment.
    Randomness randomness;

    bool operator==(const NextEpochDescriptor &rhs) const {
      return authorities == rhs.authorities and randomness == rhs.randomness;
    }
    bool operator!=(const NextEpochDescriptor &rhs) const {
      return not operator==(rhs);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const NextEpochDescriptor &ned) {
    return s << ned.authorities << ned.randomness;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, NextEpochDescriptor &ned) {
    return s >> ned.authorities >> ned.randomness;
  }

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_TYPES_NEXT_EPOCH_DESCRIPTOR_HPP
