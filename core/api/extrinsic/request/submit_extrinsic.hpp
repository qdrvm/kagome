/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_REQUEST_SUBMIT_EXTRINSIC_HPP
#define KAGOME_CORE_API_EXTRINSIC_REQUEST_SUBMIT_EXTRINSIC_HPP

#include <jsonrpc-lean/request.h>
#include "primitives/extrinsic.hpp"

namespace kagome::api {
  /**
   * @brief submitExtrinsic request parameters
   */
  struct SubmitExtrinsicRequest {
    static SubmitExtrinsicRequest fromParams(
        const jsonrpc::Request::Parameters &params);

    primitives::Extrinsic extrinsic;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_REQUEST_SUBMIT_EXTRINSIC_HPP
