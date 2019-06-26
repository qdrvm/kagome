/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/json_transport.hpp"

namespace kagome::api {

  JsonTransport::JsonTransport() {
    on_response_ = [this](const std::string &data) { processResponse(data); };
  }

}  // namespace kagome::service
