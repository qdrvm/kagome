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

#ifndef KAGOME_CORE_API_SERVICE_HPP
#define KAGOME_CORE_API_SERVICE_HPP

#include <functional>
#include <gsl/span>

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/transport/listener.hpp"
#include "common/logger.hpp"

namespace kagome::api {

  class JRpcProcessor;

  /**
   * Service listening for incoming JSON RPC requests
   */
  class ApiService : public std::enable_shared_from_this<ApiService> {
   public:
    template <class T>
    using sptr = std::shared_ptr<T>;

    /**
     * @brief constructor
     * @param context - reference to the io context
     * @param listener - a shared ptr to the endpoint listener instance
     * @param processors - shared ptrs to JSON processor instances
     */
    ApiService(std::vector<std::shared_ptr<Listener>> listeners,
               std::shared_ptr<JRpcServer> server,
               gsl::span<std::shared_ptr<JRpcProcessor>> processors);

    virtual ~ApiService() = default;
    /**
     * @brief starts service
     */
    virtual void start();

    /**
     * @brief stops service
     */
    virtual void stop();

   private:
    std::vector<sptr<Listener>> listeners_;
    std::shared_ptr<JRpcServer> server_;
    common::Logger logger_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_HPP
