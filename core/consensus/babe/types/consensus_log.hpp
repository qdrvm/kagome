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

#ifndef KAGOME_CORE_CONSENSUS_BABE_TYPES_CONSENSUS_LOG_HPP
#define KAGOME_CORE_CONSENSUS_BABE_TYPES_CONSENSUS_LOG_HPP

#include <boost/variant.hpp>

#include "consensus/babe/types/next_epoch_descriptor.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus {

  /**
   * A consensus log item for BABE.
   * The usage of AuthorityIndex option is yet unclear.
   * Name and implementation are taken from substrate.
   */
  using ConsensusLog =
      boost::variant<uint32_t,  // = 0 fake type, should never be used

                     /// The epoch has changed. This provides
                     /// information about the _next_ epoch -
                     /// information about the _current_ epoch (i.e.
                     /// the one we've just entered) should already
                     /// be available earlier in the chain.
                     NextEpochDescriptor,  // = 1

                     /// Disable the authority with given index.
                     primitives::AuthorityIndex>  // = 2 Don't know why this
                                                  // type is in this variant,
                                                  // but in substrate we have it
      ;

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_TYPES_CONSENSUS_LOG_HPP
