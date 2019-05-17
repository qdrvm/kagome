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
  struct ConnectionOpened {};
  struct ConnectionClosed {};
  struct CoffeeIsPrepared {};  // will not be emitted

  class ConnectionStateEmitter : public Emitter<ConnectionOpened, std::string>,
                                 public Emitter<ConnectionClosed, int> {
   public:
    /// without usings it's not gonna work
    //    using Emitter<ConnectionOpened, std::string>::on;
    //    using Emitter<ConnectionClosed, int>::on;
    //    using Emitter<ConnectionOpened, std::string>::emit;
    //    using Emitter<ConnectionClosed, int>::emit;

    /// way to merge those macroses into USINGS(ConnectionOpened, std::string,
    /// ConnectionClosed, int)?
    USINGS(ConnectionOpened, std::string)
    USINGS(ConnectionClosed, int)
  };
};

TEST_F(EventEmitterTest, EmitEvents) {
  ConnectionStateEmitter emitter{};

  /// only works like this, just a lambda do not; find a way to fix it?
  std::function<void(int)> handler = [](int n) { std::cout << n; };
  emitter.on<ConnectionClosed>(handler);

  emitter.emit<ConnectionClosed>(2);
}
