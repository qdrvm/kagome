/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_PROTOBUF_BLOCK_REQUEST
#define KAGOME_ADAPTERS_PROTOBUF_BLOCK_REQUEST

#include "network/adapters/protobuf.hpp"

#include "common/visitor.hpp"
#include "macro/endianness_utils.hpp"
#include "network/types/blocks_request.hpp"

namespace kagome::network {

  template <>
  struct ProtobufMessageAdapter<BlocksRequest> {
    static size_t size(const BlocksRequest &t) {
      return 0;
    }

    static std::vector<uint8_t>::iterator write(
        const BlocksRequest &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      ::api::v1::BlockRequest msg;
      msg.set_fields(LE_BE_SWAP32(t.fields.attributes.to_ulong()));

      if (t.max) msg.set_max_blocks(*t.max);
      if (t.to) msg.set_to_block(t.to->toString());

      msg.set_direction(static_cast<::api::v1::Direction>(t.direction));
      kagome::visit_in_place(
          t.from,
          [&](const primitives::BlockHash &v) { msg.set_hash(v.toString()); },
          [&](const primitives::BlockNumber &v) {
            msg.set_number(std::to_string(v));
          });

      const size_t distance_was = std::distance(out.begin(), loaded);
      const size_t was_size = out.size();

      out.resize(was_size + msg.ByteSizeLong());
      msg.SerializeToArray(&out[was_size], msg.ByteSizeLong());

      auto res_it = out.begin();
      std::advance(res_it, std::min(distance_was, was_size));
      return res_it;
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        BlocksRequest &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      assert(remains >= size(out));

      ::api::v1::BlockRequest msg;
      if (!msg.ParseFromArray(from.base(), remains))
        return AdaptersError::PARSE_FAILED;

      out.fields.load(LE_BE_SWAP32(msg.fields()));
      out.direction = static_cast<decltype(out.direction)>(msg.direction());

      switch (msg.from_block_case()) {
        case msg.kHash: {
          OUTCOME_TRY(data, primitives::BlockHash::fromString(msg.hash()));
          out.from = data;
        } break;

        case msg.kNumber: {
          out.from = std::stoull(msg.number());
        } break;

        default:
          return AdaptersError::UNEXPECTED_VARIANT;
      }

      OUTCOME_TRY(to_block, primitives::BlockHash::fromString(msg.to_block()));
      out.to = to_block;
      out.max = msg.max_blocks();

      std::advance(from, msg.ByteSizeLong());
      return from;
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF_BLOCK_REQUEST
