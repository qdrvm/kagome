/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_SERVICE_HPP
#define KAGOME_TELEMETRY_SERVICE_HPP

namespace kagome::telemetry {

  class TelemetryService {
   public:
    virtual ~TelemetryService() = default;

    virtual bool prepare() = 0;

    virtual bool start() = 0;

    virtual void stop() = 0;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_SERVICE_HPP
