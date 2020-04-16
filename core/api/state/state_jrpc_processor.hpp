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

#ifndef KAGOME_STATE_JRPC_PROCESSOR_HPP
#define KAGOME_STATE_JRPC_PROCESSOR_HPP

#include <boost/noncopyable.hpp>
#include <mutex>

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/state/state_api.hpp"

namespace kagome::api {

  class StateJrpcProcessor : public JRpcProcessor, private boost::noncopyable {
   public:
    StateJrpcProcessor(std::shared_ptr<JRpcServer> server,
                       std::shared_ptr<StateApi> api);
    ~StateJrpcProcessor() override = default;

    void registerHandlers() override;

   private:
    std::shared_ptr<StateApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api
#endif  // KAGOME_STATE_JRPC_PROCESSOR_HPP
