/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGES_MESSAGES_HPP
#define KAGOME_MESSAGES_MESSAGES_HPP

#include <boost/system/error_code.hpp>
#include <functional>
#include <gsl/span>
#include <memory>

#include "network/adapters/adapter_errors.hpp"
#include "network/protobuf/api.v1.pb.h"
#include "outcome/outcome.hpp"

namespace kagome::network {

  template <typename T, typename RW>
  struct Message {
    using ReadWriter = RW;
    T data;
  };

  template <typename... T>
  struct MessagesSequence {
    std::tuple<T...> msgs;

    template <typename Q>
    MessagesSequence(Q &&...args) : msgs{std::forward<Q>(args)...} {}
  };

}  // namespace kagome::network

#endif  // KAGOME_MESSAGES_MESSAGES_HPP
