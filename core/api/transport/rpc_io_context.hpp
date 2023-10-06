/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_RPC_IO_CONTEXT_HPP
#define KAGOME_CORE_API_RPC_IO_CONTEXT_HPP

#include <boost/asio/io_context.hpp>

namespace kagome::api {

  class RpcContext : public boost::asio::io_context {
   public:
    using boost::asio::io_context::io_context;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_RPC_IO_CONTEXT_HPP
