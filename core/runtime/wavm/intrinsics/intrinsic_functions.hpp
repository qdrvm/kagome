/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stack>

#include <WAVM/IR/Module.h>
#include <WAVM/IR/Types.h>
#include <WAVM/IR/Value.h>
#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Runtime.h>

#include "host_api/host_api.hpp"
#include "log/logger.hpp"

namespace kagome::runtime::wavm {

  class IntrinsicModule;

  void pushHostApi(std::shared_ptr<host_api::HostApi>);
  void popHostApi();
  std::shared_ptr<host_api::HostApi> peekHostApi();

  extern log::Logger logger;

  void registerHostApiMethods(IntrinsicModule &module);


}  // namespace kagome::runtime::wavm
