/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_TYPES_CONSENSUS_LOG_HPP
#define KAGOME_CORE_CONSENSUS_BABE_TYPES_CONSENSUS_LOG_HPP

#include <boost/variant.hpp>

#include "consensus/babe/types/next_epoch_descriptor.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus {

  /// An consensus log item for BABE.
  using ConsensusLog =
      boost::variant<uint32_t,  // = 0 fake type, should never be used

                     /// The epoch has changed. This provides
                     /// information about the _next_ epoch -
                     /// information about the _current_ epoch (i.e.
                     /// the one we've just entered) should already
                     /// be available earlier in the chain.
                     NextEpochDescriptor,  // = 1

                     /// Disable the authority with given index.
                     primitives::AuthorityIndex>  // = 2
      ;

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_TYPES_CONSENSUS_LOG_HPP
