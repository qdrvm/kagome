/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
#define KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP

#include <cstdint>

namespace kagome::primitives {

  enum struct SubscriptionEventType : uint32_t {
    kNewHeads = 1
  };

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_EVENT_TYPES_HPP
