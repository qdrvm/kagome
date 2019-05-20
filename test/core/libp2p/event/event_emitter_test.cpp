/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/event/emitter.hpp"

#include <gtest/gtest.h>

using namespace libp2p::event;

class EventEmitterTest : public ::testing::Test {
 public:
  /// events, which are to be emitted by the test emitter
  struct ConnectionOpened {
    std::string my_str;
  };
  struct ConnectionClosed {
    int code1;
    int code2;
  };
  struct CoffeeIsPrepared {};  // compile failure if tried to be emitted

  struct ConnectionStateEmitter {
    KAGOME_EMITS(ConnectionOpened)
    KAGOME_EMITS(ConnectionClosed)
  };
};

/**
 * @given event emitter with two events
 * @when subscribing to those events @and emitting them
 * @then events are successfully emitted
 */
TEST_F(EventEmitterTest, EmitEvents) {
  std::string connection_opened{};
  auto connection_closed_n = -1;
  auto connection_closed_k = -1;

  ConnectionStateEmitter emitter{};

  emitter.on([&connection_opened](ConnectionOpened event) {
    connection_opened = event.my_str;
  });

  emitter.on(
      [&connection_closed_n, &connection_closed_k](ConnectionClosed event) {
        connection_closed_n = event.code1;
        connection_closed_k = event.code2;
      });

  emitter.emit(ConnectionOpened{"foo"});
  emitter.emit(ConnectionClosed{2, 5});

  ASSERT_EQ(connection_opened, "foo");
  ASSERT_EQ(connection_closed_n, 2);
  ASSERT_EQ(connection_closed_k, 5);
}
