/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STREAM_HPP
#define KAGOME_CONNECTION_STREAM_HPP

#include "libp2p/basic/readwritecloser.hpp"

namespace libp2p::connection {

  struct Stream : public basic::ReadWriteCloser {
    using Handler = void(std::shared_ptr<Stream>);

    ~Stream() override = default;

    /**
     * Break the connection entirely, so that no writes or reads can be done
     */
    virtual void reset() = 0;

    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const = 0;

    /**
     * Check, if this stream is closed from this side of the connection and thus
     * cannot be written to
     * @return true, of stream cannot be written to, false otherwise
     */
    virtual bool isClosedForWrite() const = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_STREAM_HPP
