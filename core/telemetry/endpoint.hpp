/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_ENDPOINT_HPP
#define KAGOME_TELEMETRY_ENDPOINT_HPP

#include "common/uri.hpp"

namespace kagome::telemetry {

  /**
   * Represents a single record of telemetry URI endpoint
   * with verbosity level specified
   */
  class TelemetryEndpoint {
   public:
    TelemetryEndpoint(common::Uri endpoint_uri, uint8_t verbosity_level)
        : uri_{std::move(endpoint_uri)}, verbosity_level_{verbosity_level} {};

    const common::Uri &uri() const {
      return uri_;
    };

    uint8_t verbosity() {
      return verbosity_level_;
    };

    bool operator==(const TelemetryEndpoint &other) const {
      return uri_.to_string() == other.uri_.to_string()
             and verbosity_level_ == other.verbosity_level_;
    }

    bool operator!=(const TelemetryEndpoint &other) const {
      return not operator==(other);
    }

   private:
    common::Uri uri_;
    uint8_t verbosity_level_;
  };
}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_ENDPOINT_HPP
