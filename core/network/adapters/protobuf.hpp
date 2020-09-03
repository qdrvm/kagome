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
    static size_t size(const T &t) {
      assert(false);  // NO_IMPL
      return 0ull;
    }
    static std::vector<uint8_t>::iterator write(
        const T & /*t*/,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator /*loaded*/) {
      assert(false);  // NO_IMPL
      return out.end();
    }
    static libp2p::outcome::result<std::vector<uint8_t>::const_iterator> read(
        T &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      assert(false);  // NO_IMPL
      return outcome::failure(boost::system::error_code{});
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF
