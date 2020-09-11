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

#include "network/protobuf/api.v1.pb.h"
#include "outcome/outcome.hpp"

#ifdef _MSC_VER
# define LE_BE_SWAP32 _byteswap_ulong
# define LE_BE_SWAP64 _byteswap_uint64
#else//_MSC_VER
# define LE_BE_SWAP32 __builtin_bswap32
# define LE_BE_SWAP64 __builtin_bswap64
#endif//_MSC_VER

namespace kagome::network {

  template <typename T>
  struct ProtobufMessageAdapter {
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF
