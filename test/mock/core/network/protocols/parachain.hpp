/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/impl/protocols/parachain.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  class ValidationProtocolReserveMock : public ValidationProtocolReserve {
   public:
    MOCK_METHOD(void, reserve, (const PeerId &, bool), (override));
  };
}  // namespace kagome::network
