/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/babe_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class BabeApiMock : public BabeApi {
   public:
    MOCK_METHOD(outcome::result<consensus::babe::BabeConfiguration>,
                configuration,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<consensus::babe::Epoch>,
                next_epoch,
                (const primitives::BlockHash &),
                (override));
  };

}  // namespace kagome::runtime
