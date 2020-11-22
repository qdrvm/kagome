/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
#define KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP

#include <cstdint>
#include <memory>

#include <libp2p/peer/peer_id.hpp>
#include <boost/none_t.hpp>
#include <boost/variant.hpp>

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"

#include "primitives/version.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::api {
  class Session;
}  // namespace kagome::api

namespace kagome::primitives {
  struct BlockHeader;
}

namespace kagome::primitives::events {

  enum struct ChainEventType : uint32_t {
    kNewHeads = 1,
    kFinalizedHeads = 2,
    kAllHeads = 3,
    kRuntimeVersion = 4
  };

  enum class ExtrinsicLifecycleEvent {
    FUTURE,
    READY,
    BROADCAST,
    IN_BLOCK,
    RETRACTED,
    FINALITY_TIMEOUT,
    FINALIZED,
    USURPED,
    DROPPED,
    INVALID
  };

  struct BroadcastEventParams {
    std::vector<libp2p::peer::PeerId> peers;
  };

  struct InBlockEventParams {
    primitives::BlockHash block;
  };
  struct RetractedEventParams {
    primitives::BlockHash retracted_block;
  };

  struct FinalityTimeoutEventParams {
    primitives::BlockHash block;
  };

  struct FinalizedEventParams {
    primitives::BlockHash block;
  };

  struct UsurpedEventParams {
    common::Hash256 transaction_hash;
  };

  template <typename T>
  using ref = std::reference_wrapper<T>;

  using ExtrinsicLifecycleEventParams =
      std::variant<boost::none_t,
                   ref<BroadcastEventParams>,
                   ref<InBlockEventParams>,
                   ref<RetractedEventParams>,
                   ref<FinalityTimeoutEventParams>,
                   ref<FinalizedEventParams>,
                   ref<UsurpedEventParams>>;

  using ChainEventParams = boost::variant<boost::none_t,
                                          ref<const primitives::BlockHeader>,
                                          ref<primitives::Version>>;

}  // namespace kagome::primitives::events

#endif  // KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
