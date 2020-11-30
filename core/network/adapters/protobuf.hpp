/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_PROTOBUF
#define KAGOME_ADAPTERS_PROTOBUF

#include <functional>
#include <memory>
#include <gsl/span>
#include <boost/system/error_code.hpp>

#include "network/adapters/adapter_errors.hpp"
#include "network/protobuf/api.v1.pb.h"
#include "outcome/outcome.hpp"

namespace kagome::network {

  template <typename T>
  struct ProtobufMessageAdapter {};

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF
