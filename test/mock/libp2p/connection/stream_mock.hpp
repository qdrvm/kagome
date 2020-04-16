/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LIBP2P_STREAM_MOCK_HPP
#define LIBP2P_STREAM_MOCK_HPP

#include <gmock/gmock.h>
#include <libp2p/connection/stream.hpp>

namespace libp2p::connection {
  class StreamMock : public Stream {
   public:
    ~StreamMock() override = default;

    StreamMock() = default;
    explicit StreamMock(uint8_t id) : stream_id{id} {}

    /// this field here is for easier testing
    uint8_t stream_id = 137;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD1(close, void(VoidResultHandlerFunc));

    MOCK_METHOD3(read,
                 void(gsl::span<uint8_t>, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(readSome,
                 void(gsl::span<uint8_t>, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(write,
                 void(gsl::span<const uint8_t>,
                      size_t,
                      Writer::WriteCallbackFunc));
    MOCK_METHOD3(writeSome,
                 void(gsl::span<const uint8_t>,
                      size_t,
                      Writer::WriteCallbackFunc));

    MOCK_METHOD0(reset, void());

    MOCK_CONST_METHOD0(isClosedForRead, bool(void));

    MOCK_CONST_METHOD0(isClosedForWrite, bool(void));

    MOCK_METHOD2(adjustWindowSize, void(uint32_t, VoidResultHandlerFunc));

    MOCK_CONST_METHOD0(isInitiator, outcome::result<bool>());

    MOCK_CONST_METHOD0(remotePeerId, outcome::result<peer::PeerId>());

    MOCK_CONST_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());

    MOCK_CONST_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };
}  // namespace libp2p::connection

#endif  // LIBP2P_STREAM_MOCK_HPP
