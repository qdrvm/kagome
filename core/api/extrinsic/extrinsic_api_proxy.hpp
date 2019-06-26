/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_PROXY_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_PROXY_HPP

#include <vector>

#include <jsonrpc-lean/fault.h>
#include <jsonrpc-lean/value.h>
#include "api/extrinsic/extrinsic_api.hpp"
#include "common/buffer.hpp"

namespace kagome::api {

  /**
   * @brief ExtrinsicSubmissionProxy decodes json-serialized params and calls
   * underlying api
   */
  class ExtrinsicApiProxy {
    template <class T>
    using sptr = std::shared_ptr<T>;

   public:
    /**
     * @brief proxy between extrinsic submission server and api
     * @param api reference to extrinsic submission api instance
     */
    explicit ExtrinsicApiProxy(std::shared_ptr<ExtrinsicApi> api);

    /**
     * @brief calls submit_extrinsic api method
     * @param hexified_extrinsic hex-encoded extrinsic
     * @return extrinsic hash as vector
     */
    std::vector<uint8_t> submit_extrinsic(
        const jsonrpc::Value::String &hexified_extrinsic);

    /**
     * @brief calls pending_extrinsic api method
     * @return vector of pending extrinsics
     */
    std::vector<std::vector<uint8_t>> pending_extrinsics();

    // other methods will be implemented later

   private:
    sptr<ExtrinsicApi> api_;  ///< pointer to extrinsic api instance
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_PROXY_HPP
