/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

#include <jsonrpc-lean/server.h>

#include <boost/core/noncopyable.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "api/extrinsic/extrinsic_api.hpp"
#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"

namespace kagome::api {

  /**
   * @brief extrinsic submission service implementation
   */
  class ExtrinsicJRpcProcessor : public JRpcProcessor,
                                 private boost::noncopyable {
   public:
    ExtrinsicJRpcProcessor(std::shared_ptr<JRpcServer> server,
                           std::shared_ptr<ExtrinsicApi> api);

   private:
    void registerHandlers() override;

    std::shared_ptr<ExtrinsicApi> api_;  ///< api implementation
    std::shared_ptr<JRpcServer> server_;
    std::mutex mutex_{};                 ///< mutex for synchronization
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
