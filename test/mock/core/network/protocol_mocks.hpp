/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_fetch_chunk_obsolete.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"

#include "mock/core/network/protocol_base_mock.hpp"
#include "mock/core/network/protocol_mocks.hpp"

namespace kagome::network {

  template <class ProtocolInterface>
    requires(requires {
              typename ProtocolInterface::RequestType;
              typename ProtocolInterface::ResponseType;
            })
  class RequestResponseProtocolMock
      : public ProtocolInterface,
        public ProtocolBaseMock,
        virtual public RequestResponseProtocol<
            typename ProtocolInterface::RequestType,
            typename ProtocolInterface::ResponseType> {
   public:
    MOCK_METHOD(
        void,
        doRequest,
        (const PeerId &,
         typename ProtocolInterface::RequestType,
         std::function<void(
             outcome::result<typename ProtocolInterface::ResponseType>)> &&),
        ());
  };

  using FetchChunkProtocolMock =
      RequestResponseProtocolMock<FetchChunkProtocol>;

  using FetchChunkProtocolObsoleteMock =
      RequestResponseProtocolMock<FetchChunkProtocolObsolete>;

  using FetchAvailableDataProtocolMock =
      RequestResponseProtocolMock<FetchAvailableDataProtocol>;

}  // namespace kagome::network
