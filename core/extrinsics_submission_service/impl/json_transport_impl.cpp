/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "extrinsics_submission_service/impl/json_transport_impl.hpp"

namespace kagome::service {
  JsonTransportImpl::JsonTransportImpl(
      boost::asio::io_context &context,
      ExtrinsicSubmissionServiceConfiguration config)
      : JsonTransport(
          [this](const std::string &data) { processResponse(data); }) {}

  void JsonTransportImpl::processResponse(const std::string &response) {
    // TODO(yuraz): PRE-207 implement
  }
}  // namespace kagome::service
