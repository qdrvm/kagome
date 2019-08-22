/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_MOCK_HPP
#define KAGOME_MESSAGE_READ_WRITER_MOCK_HPP

#include <memory>

#include <gmock/gmock.h>
#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/basic/readwriter.hpp"

namespace libp2p::basic {
  struct IMessageReadWriter {
    virtual ~IMessageReadWriter() = default;

    virtual void read(MessageReadWriter::ReadCallbackFunc cb) = 0;

    virtual void write(gsl::span<const uint8_t> buffer,
                       Writer::WriteCallbackFunc cb) = 0;
  };

  struct MessageReadWriterMock : public IMessageReadWriter {
    explicit MessageReadWriterMock(std::shared_ptr<ReadWriter> conn)
        : conn_{std::move(conn)} {}
    ~MessageReadWriterMock() override = default;

    MOCK_METHOD1(read, void(MessageReadWriter::ReadCallbackFunc));

    MOCK_METHOD2(write,
                 void(gsl::span<const uint8_t>, Writer::WriteCallbackFunc));

   private:
    std::shared_ptr<ReadWriter> conn_;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_MESSAGE_READ_WRITER_MOCK_HPP
