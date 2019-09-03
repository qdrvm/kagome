/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <vector>

#include <jsonrpc-lean/server.h>
#include <boost/core/noncopyable.hpp>
#include <boost/signals2/signal.hpp>
#include "api/extrinsic/extrinsic_api.hpp"
#include "api/jrpc/jrpc_processor.hpp"
#include "api/transport/impl/listener_impl.hpp"

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
    ExtrinsicJRPCProcessor(std::shared_ptr<ExtrinsicApi> api);

   private:
    std::shared_ptr<ExtrinsicApi> api_;  ///< api implementation
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
