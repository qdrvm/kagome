/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE
#define KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE

#include "network/adapters/protobuf.hpp"

namespace kagome::network {

  template <>
  struct ProtobufMessageAdapter<network::BlocksResponse> {
    static size_t size(const network::BlocksResponse &t) {
      return 0;
    }
    static std::vector<uint8_t>::iterator write(
        const network::BlocksResponse &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      return out.end();
    }

    static libp2p::outcome::result<std::vector<uint8_t>::const_iterator> read(
        network::BlocksResponse &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      return outcome::failure(boost::system::error_code{});
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE
