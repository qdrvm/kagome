/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_SERVICE_MOCK_HPP
#define KAGOME_TELEMETRY_SERVICE_MOCK_HPP

#include <gmock/gmock.h>

#include "telemetry/service.hpp"

namespace kagome::telemetry {

  class TelemetryServiceMock : public TelemetryService {
    MOCK_METHOD(std::string, connectedMessage, (), (const override));

    MOCK_METHOD(void,
                blockImported,
                (const std::string &, uint32_t),
                (override));

    MOCK_METHOD(bool, prepare, (), (override));

    MOCK_METHOD(bool, start, (), (override));

    MOCK_METHOD(void, stop, (), (override));
  };
}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_SERVICE_MOCK_HPP
