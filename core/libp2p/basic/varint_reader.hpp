/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VARINT_READER_HPP
#define KAGOME_VARINT_READER_HPP

#include <functional>
#include <memory>
#include <optional>

#include "common/buffer.hpp"
#include "libp2p/basic/readwriter.hpp"
#include "libp2p/multi/uvarint.hpp"

namespace libp2p::basic {
  class VarintReader {
   public:
    static constexpr uint8_t kMaximumVarintLength = 9;  // taken from Go

    /**
     * Read a varint from the connection
     * @param conn to be read from
     * @param cb to be called, when a varint or maximum bytes from the
     * connection are read
     */
    static void readVarint(
        std::shared_ptr<ReadWriter> conn,
        std::function<void(std::optional<multi::UVarint>)> cb) {
      readVarint(
          std::move(conn), std::move(cb), 0,
          std::make_shared<kagome::common::Buffer>(kMaximumVarintLength, 0));
    }

   private:
    static void readVarint(
        std::shared_ptr<ReadWriter> conn,
        std::function<void(std::optional<multi::UVarint>)> cb,
        uint8_t current_length,
        std::shared_ptr<kagome::common::Buffer> varint_buf) {
      if (current_length > kMaximumVarintLength) {
        return cb(std::nullopt);
      }

      conn->read(gsl::make_span(varint_buf->data() + current_length, 1), 1,
                 [c = std::move(conn), cb = std::move(cb), current_length,
                  varint_buf](auto &&res) mutable {
                   if (!res) {
                     return cb(std::nullopt);
                   }

                   auto varint_opt = multi::UVarint::create(*varint_buf);
                   if (varint_opt) {
                     return cb(*varint_opt);
                   }

                   readVarint(std::move(c), std::move(cb), ++current_length,
                              std::move(varint_buf));
                 });
    }
  };
}  // namespace libp2p::basic

#endif  // KAGOME_VARINT_READER_HPP
