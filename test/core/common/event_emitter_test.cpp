/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/event_emitter.hpp"

#include <gtest/gtest.h>

using namespace kagome::common;

class EventEmitterTest : public ::testing::Test {
 public:
  /// events, which are to be emitted by the test emitter
  struct ConnectionOpened {};
  struct ConnectionClosed {};
  struct CoffeeIsPrepared {};  // will not be emitted

  class ConnectionStateEmitter
      : public EventEmitter<ConnectionOpened, std::string>,
        public EventEmitter<ConnectionClosed, int> {
   public:
    template <typename Tag, typename... EventArgs>
    void on(const std::function<void(EventArgs...)> &handler) {
      EventEmitter<Tag, EventArgs...>::subscribe(handler);
    }

    template <typename Tag, typename... EventArgs>
    void emit(EventArgs &&... args) {
      EventEmitter<Tag, EventArgs...>::fire(
          std::forward<EventArgs...>(args...));
    }
  };
};

TEST_F(EventEmitterTest, EmitEvents) {
  ConnectionStateEmitter emitter{};

  std::function<void(int)> handler = [](int n) { std::cout << n; };
  emitter.on<ConnectionClosed>(handler);
  emitter.emit<ConnectionClosed>(2);
}
