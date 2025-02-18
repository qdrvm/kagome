/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/session_keys_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  struct SessionKeysApiMock : public SessionKeysApi {
    MOCK_METHOD(outcome::result<common::Buffer>,
                generate_session_keys,
                (const primitives::BlockHash &, std::optional<common::Buffer>),
                (override));

    MOCK_METHOD(outcome::result<DecodeSessionKeysResult>,
                decode_session_keys,
                (const primitives::BlockHash &, common::BufferView),
                (const, override));
  };

}  // namespace kagome::runtime
