/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace kagome {
  using WeakIoContext = std::weak_ptr<boost::asio::io_context>;
}  // namespace kagome
