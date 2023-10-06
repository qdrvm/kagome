/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/author/author_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/service/author/requests/has_key.hpp"
#include "api/service/author/requests/has_session_keys.hpp"
#include "api/service/author/requests/insert_key.hpp"
#include "api/service/author/requests/pending_extrinsics.hpp"
#include "api/service/author/requests/rotate_keys.hpp"
#include "api/service/author/requests/submit_and_watch_extrinsic.hpp"
#include "api/service/author/requests/submit_extrinsic.hpp"
#include "api/service/author/requests/unwatch_extrinsic.hpp"

namespace kagome::api::author {

  AuthorJRpcProcessor::AuthorJRpcProcessor(std::shared_ptr<JRpcServer> server,
                                           std::shared_ptr<AuthorApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = kagome::api::Method<Request, AuthorApi>;

  void AuthorJRpcProcessor::registerHandlers() {
    server_->registerHandler("author_submitExtrinsic",
                             Handler<request::SubmitExtrinsic>(api_));

    server_->registerHandlerUnsafe("author_insertKey",
                                   Handler<request::InsertKey>(api_));

    server_->registerHandlerUnsafe("author_hasSessionKeys",
                                   Handler<request::HasSessionKeys>(api_));

    server_->registerHandlerUnsafe("author_hasKey",
                                   Handler<request::HasKey>(api_));

    server_->registerHandlerUnsafe("author_rotateKeys",
                                   Handler<request::RotateKeys>(api_));

    server_->registerHandler("author_submitAndWatchExtrinsic",
                             Handler<request::SubmitAndWatchExtrinsic>(api_));

    server_->registerHandler("author_unwatchExtrinsic",
                             Handler<request::UnwatchExtrinsic>(api_));

    server_->registerHandler("author_pendingExtrinsics",
                             Handler<request::PendingExtrinsics>(api_));
  }

}  // namespace kagome::api::author
