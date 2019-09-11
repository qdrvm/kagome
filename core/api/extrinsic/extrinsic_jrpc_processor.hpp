/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <memory>
#include <mutex>
#include <vector>

#include <jsonrpc-lean/server.h>
#include <boost/core/noncopyable.hpp>
#include "api/extrinsic/extrinsic_api.hpp"
#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  /**
   * @brief extrinsic submission service implementation
   */
  class ExtrinsicJRPCProcessor : public JRPCProcessor,
                                 private boost::noncopyable {
   public:
    /**
     * @brief constructor
     */
    explicit ExtrinsicJRPCProcessor(std::shared_ptr<ExtrinsicApi> api);

   private:
    void registerHandlers() override;

    std::shared_ptr<ExtrinsicApi> api_;  ///< api implementation
    std::mutex mutex_{};                 ///< mutex for synchronization
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
