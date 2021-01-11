/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE
#define KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE

#include "network/adapters/protobuf.hpp"

#include "network/types/blocks_response.hpp"
#include "scale/scale.hpp"

namespace kagome::network {

  template <>
  struct ProtobufMessageAdapter<BlocksResponse> {
    static size_t size(const BlocksResponse &t) {
      return 0;
    }

    static std::vector<uint8_t>::iterator write(
        const BlocksResponse &t,
        std::vector<uint8_t> &out,
        std::vector<uint8_t>::iterator loaded) {
      ::api::v1::BlockResponse msg;
      for (const auto &src_block : t.blocks) {
        auto *dst_block = msg.add_blocks();
        dst_block->set_hash(src_block.hash.toString());

        if (src_block.header)
          dst_block->set_header(
              vector_to_string(scale::encode(*src_block.header).value()));

        if (src_block.body)
          for (const auto &ext_body : *src_block.body)
            dst_block->add_body(
                vector_to_string(scale::encode(ext_body).value()));

        if (src_block.receipt)
          dst_block->set_receipt(src_block.receipt->asString());

        if (src_block.message_queue)
          dst_block->set_message_queue(src_block.message_queue->asString());

        if (src_block.justification) {
          dst_block->set_justification(
              src_block.justification->data.asString());

          dst_block->set_is_empty_justification(
              src_block.justification->data.empty());
        };
      }

      const size_t distance_was = std::distance(out.begin(), loaded);
      const size_t was_size = out.size();

      out.resize(was_size + msg.ByteSizeLong());
      msg.SerializeToArray(&out[was_size], msg.ByteSizeLong());

      auto res_it = out.begin();
      std::advance(res_it, std::min(distance_was, was_size));
      return res_it;
    }

    static outcome::result<std::vector<uint8_t>::const_iterator> read(
        BlocksResponse &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      assert(remains >= size(out));

      ::api::v1::BlockResponse msg;
      if (!msg.ParseFromArray(from.base(), remains))
        return AdaptersError::PARSE_FAILED;

      auto &dst_blocks = out.blocks;
      dst_blocks.reserve(msg.blocks().size());
      for (const auto &src_block_data : msg.blocks()) {
        OUTCOME_TRY(hash,
                    primitives::BlockHash::fromString(src_block_data.hash()));

        OUTCOME_TRY(header, extract_value<primitives::BlockHeader>([&]() {
                      return src_block_data.header();
                    }));

        boost::optional<primitives::BlockBody> bodies;
        for (const auto &b : src_block_data.body()) {
          if (!bodies) bodies = primitives::BlockBody{};

          OUTCOME_TRY(
              body, extract_value<primitives::Extrinsic>([&]() { return b; }));
          bodies->emplace_back(std::move(body));
        }

        OUTCOME_TRY(receipt,
                    common::Buffer::fromString(src_block_data.receipt()));

        OUTCOME_TRY(message_queue,
                    common::Buffer::fromString(src_block_data.message_queue()));


        boost::optional<primitives::Justification> justification;

        if (not src_block_data.justification().empty()
            || src_block_data.is_empty_justification()) {
          OUTCOME_TRY(data,
                      common::Buffer::fromString(src_block_data.justification()));

          justification.emplace(
              primitives::Justification{std::move(data)});
        }

        dst_blocks.emplace_back(
            primitives::BlockData{.hash = hash,
                                  .header = std::move(header),
                                  .body = std::move(bodies),
                                  .receipt = std::move(receipt),
                                  .message_queue = std::move(message_queue),
                                  .justification = std::move(justification)});
      }

      std::advance(from, msg.ByteSizeLong());
      return from;
    }

   private:
    template <typename T, typename F>
    static outcome::result<T> extract_value(F &&f) {
      if (const auto &buffer = std::forward<F>(f)(); !buffer.empty()) {
        OUTCOME_TRY(
            decoded,
            scale::decode<T>(gsl::span<const uint8_t>(
                reinterpret_cast<const uint8_t *>(buffer.data()),  // NOLINT
                buffer.size())));
        return std::move(decoded);
      }
      return AdaptersError::EMPTY_DATA;
    }

    static std::string vector_to_string(std::vector<uint8_t> &&src) {
      return std::string(reinterpret_cast<char *>(src.data()),  // NOLINT
                         src.size());
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE
