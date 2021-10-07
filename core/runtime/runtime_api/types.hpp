/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TYPES
#define KAGOME_RUNTIME_TYPES

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::runtime {

  using Timestamp = uint64_t;
  using RandomSeed = common::Blob<32>;
  enum class KindStorage : uint32_t { Persistent = 1, Local = 2 };
  enum class Method { GET = 1, POST = 2 };
  using RequestId = int16_t;
  enum class HttpError {
    Timeout = 0,   //!< The deadline was reached
    IoError = 1,   //!< There was an IO error while processing the request
    InvalidId = 2  //!< The ID of the request is invalid
  };

  struct Success {};
  struct Failure {};

  struct OpaqueNetworkState {
    libp2p::peer::PeerId peer_id;
    std::list<libp2p::multi::Multiaddress> address;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_OFFCHAINAPI
