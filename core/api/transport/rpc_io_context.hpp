/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

namespace kagome::api {

  class RpcContext : public boost::asio::io_context {
   public:
    using boost::asio::io_context::io_context;
  };

}  // namespace kagome::api
