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
      api::v1::BlockResponse msg;
      for (const auto &src_block : t.blocks) {
        auto dst_block = msg.add_blocks();
        dst_block->set_hash(src_block.hash.toHex());

        if (src_block.header)
          dst_block->set_header(
              vector_to_string(scale::encode(src_block.header).value()));

        if (src_block.body)
          for (auto &ext_body : *src_block.body)
            dst_block->add_body(
                vector_to_string(scale::encode(ext_body).value()));

        if (src_block.receipt)
          dst_block->set_receipt(src_block.receipt->toHex());

        if (src_block.message_queue)
          dst_block->set_message_queue(src_block.message_queue->toHex());

        if (src_block.justification)
          dst_block->set_justification(src_block.justification->data.toHex());
      }

      const size_t distance_was = std::distance(out.begin(), loaded);
      const size_t was_size = out.size();

      out.resize(was_size + msg.ByteSizeLong());
      msg.SerializeToArray(&out[was_size], msg.ByteSizeLong());

      auto res_it = out.begin();
      std::advance(res_it, std::min(distance_was, was_size));
      return std::move(res_it);
    }

    static libp2p::outcome::result<std::vector<uint8_t>::const_iterator> read(
        network::BlocksResponse &out,
        const std::vector<uint8_t> &src,
        std::vector<uint8_t>::const_iterator from) {
      const auto remains = src.size() - std::distance(src.begin(), from);
      assert(remains >= size(out));

      api::v1::BlockResponse msg;
      if (!msg.ParseFromArray(from.base(), remains))
        return outcome::failure(boost::system::error_code{});

      auto &dst_blocks = out.blocks;
      dst_blocks.reserve(msg.blocks().size());
      for (const auto &src_block_data : msg.blocks()) {
        OUTCOME_TRY(hash,
                    primitives::BlockHash::fromHex(src_block_data.hash()));

        OUTCOME_TRY(header, extract_value<primitives::BlockHeader>([&]() {
                      return src_block_data.header();
                    }));

        OUTCOME_TRY(body, extract_value<primitives::BlockBody>([&]() {
                      return src_block_data.body();
                    }));

        OUTCOME_TRY(receipt, common::Buffer::fromHex(src_block_data.receipt()));

        OUTCOME_TRY(message_queue,
                    common::Buffer::fromHex(src_block_data.message_queue()));

        OUTCOME_TRY(justification,
                    common::Buffer::fromHex(src_block_data.justification()));

        dst_blocks.emplace_back(primitives::BlockData{
            .hash = hash,
            .header = std::move(header),
            .body = std::move(body),
            .receipt = std::move(receipt),
            .message_queue = std::move(message_queue),
            .justification =
                primitives::Justification{std::move(justification)}});
      }

      return outcome::success();
    }

   private:
    template<typename T, typename F>
    static libp2p::outcome::result<T> extract_value(F &&f) {
      if (const auto &buffer = std::forward<F>(f)(); !buffer.empty()) {
        OUTCOME_TRY(decoded,
                    scale::decode<T>(gsl::span<const uint8_t>(
                        reinterpret_cast<const uint8_t *>(buffer.data()),
                        buffer.size())));
        return std::move(decoded);
      }
      return outcome::failure(boost::system::error_code{});
    }

    static std::string vector_to_string(std::vector<uint8_t> &&src) {
      return std::string(reinterpret_cast<char *>(src.data()), src.size());
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_ADAPTERS_PROTOBUF_BLOCK_RESPONSE
