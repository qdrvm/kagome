/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/router_impl.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/connection/stream_mock.hpp"

using namespace libp2p::network;
using namespace libp2p::peer;
using namespace libp2p::connection;

class RouterTest : public ::testing::Test, public RouterImpl {
 public:
  static constexpr uint8_t kDefaultStreamId = 5;
  const std::shared_ptr<Stream> kStreamToSend =
      std::make_shared<StreamMock>(kDefaultStreamId);
  std::shared_ptr<Stream> stream_to_receive;

  const Protocol kDefaultProtocol = "ping/1.5.2";

  /**
   * Knowing that a provided stream is a mocked one, get its ID
   * @param stream - mock stream
   * @return id of the stream
   */
  uint8_t getStreamMockId(const Stream &stream) {
    return dynamic_cast<const StreamMock &>(stream).stream_id;
  }
};

/**
 * @given router @and protocol to be handled
 * @when setting a perfect-match handler for that protocol @and calling handle
 * @then the corresponding handler is invoked
 */
TEST_F(RouterTest, SetHandlerPerfect) {
  this->setProtocolHandler(kDefaultProtocol,
                           [this](std::shared_ptr<Stream> stream) mutable {
                             stream_to_receive = std::move(stream);
                           });

  EXPECT_TRUE(this->handle(kDefaultProtocol, kStreamToSend));
  EXPECT_TRUE(stream_to_receive);
  EXPECT_EQ(getStreamMockId(*kStreamToSend),
            getStreamMockId(*stream_to_receive));
}

/**
 * @given router @and protocol to be handled
 * @when setting a set of predicate-match handlers for that protocol @and
 * calling handle
 * @then the corresponding handler is invoked
 */
TEST_F(RouterTest, SetHandlerWithPredicate) {
  /// this match is shorter, than the next two; must not be invoked
  this->setProtocolHandler(
      kDefaultProtocol.substr(0, 5),  // 'ping/'
      [](auto &&) { FAIL(); }, [](auto &&) { return true; });

  /// this match is equal to the next one, but its handler will evaluate to
  /// false; must not be invoked
  this->setProtocolHandler(
      kDefaultProtocol.substr(0, 8),  // 'ping/1.5'
      [](auto &&) { FAIL(); }, [](auto &&) { return false; });

  /// this match must be invoked
  this->setProtocolHandler(
      kDefaultProtocol.substr(0, 8),  // 'ping/1.5'
      [this](std::shared_ptr<Stream> stream) mutable {
        stream_to_receive = std::move(stream);
      },
      [this](const auto &proto) { return proto == kDefaultProtocol; });

  EXPECT_TRUE(this->handle(kDefaultProtocol, kStreamToSend));
  EXPECT_TRUE(stream_to_receive);
  EXPECT_EQ(getStreamMockId(*kStreamToSend),
            getStreamMockId(*stream_to_receive));
}

TEST_F(RouterTest, GetSupportedProtocols) {}

TEST_F(RouterTest, RemoveProtocolHandlers) {}

TEST_F(RouterTest, RemoveAll) {}
