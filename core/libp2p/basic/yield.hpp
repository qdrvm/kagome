/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YIELD_HPP
#define KAGOME_YIELD_HPP

#include <boost/asio/io_context.hpp>
#include <ufiber/ufiber.hpp>

namespace libp2p::basic {

  using yield_t = ufiber::yield_token<boost::asio::io_context::executor_type>;

}

#endif  // KAGOME_YIELD_HPP
