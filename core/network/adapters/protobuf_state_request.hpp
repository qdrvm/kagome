/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_PROTOBUF_STATE_REQUEST
#define KAGOME_ADAPTERS_PROTOBUF_STATE_REQUEST

#include "network/adapters/protobuf.hpp"

#include <gsl/span>
#include <libp2p/multi/uvarint.hpp>

#include "common/visitor.hpp"
#include "macro/endianness_utils.hpp"
#include "network/protobuf/api.v1.pb.h"
#include "network/types/state_request.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  template <>
  struct ProtobufMessageAdapter<StateRequest> {
    static size_t size(const StateRequest &t) {
      return 0;
    }

    static std::vector<uint8_t>::iterator write(
        const StateRequest &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      ::api::v1::StateRequest msg;

      msg.set_block(t.hash.toString());
      for (const auto &start : t.start) {
        msg.add_start(start.toString());
      }
      msg.set_no_proof(t.no_proof);

      return appendToVec(msg, out, loaded);
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        StateRequest &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      assert(remains >= size(out));

      ::api::v1::StateRequest msg;
      if (!msg.ParseFromArray(from.base(), remains)) {
        return AdaptersError::PARSE_FAILED;
      }

      auto hash = primitives::BlockHash::fromString(msg.block());
      if (hash.has_error()) {
        return AdaptersError::CAST_FAILED;
      }
      out.hash = std::move(hash.value());

      for (const auto &strt : msg.start()) {
        out.start.push_back(common::Buffer::fromString(strt));
      }

      out.no_proof = msg.no_proof();

      std::advance(from, msg.ByteSizeLong());
      return from;
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF_STATE_REQUEST
