/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/json_transport.hpp"

namespace kagome::service {

  JsonTransport::JsonTransport()
      : on_response_{[this](std::string_view data) { processResponse(data); }} {
  }

  void JsonTransport::processResponse(std::string_view response) {}

  // TODO(yuraz): PRE-207 implement
  outcome::result<void> JsonTransport::start(NetworkAddress address) {
    std::terminate();
  }

  void JsonTransport::stop() {}

}  // namespace kagome::service
