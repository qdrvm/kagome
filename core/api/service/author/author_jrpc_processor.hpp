/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/author/author_api.hpp"

namespace kagome::api::author {

  /**
   * @brief extrinsic submission service implementation
   */
  class AuthorJRpcProcessor : public JRpcProcessor {
   public:
    AuthorJRpcProcessor(std::shared_ptr<JRpcServer> server,
                        std::shared_ptr<AuthorApi> api);

   private:
    void registerHandlers() override;

    std::shared_ptr<AuthorApi> api_;
    std::shared_ptr<JRpcServer> server_;
  };

}  // namespace kagome::api::author
