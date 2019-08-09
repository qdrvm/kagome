/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <iostream>

///// https://github.com/boost-experimental/sml

/// include
#include <boost/sml.hpp>
namespace sml = boost::sml;

/// dependencies
struct sender {
  template <class TMsg>
  constexpr void send(const TMsg &msg) {
    std::cout << "send: " << msg.id << '\n';
  }
};

/// events
struct ack {
  bool valid{};
};
struct fin {
  int id{};
  bool valid{};
};
struct release {};
struct timeout {};

/// guards
constexpr auto is_valid = [](const auto &event) { return event.valid; };

/// actions
constexpr auto send_fin = [](sender &s) { s.send(fin{0}); };
constexpr auto send_ack = [](const auto &event, sender &s) { s.send(event); };

/// state machine
struct tcp_release {
  auto operator()() const {
    using namespace sml;
    /**
     * Initial state: *initial_state
     * Transition DSL: src_state + event [ guard ] / action = dst_state
     */
    return make_transition_table(
        *"established"_s + event<release> / send_fin = "fin wait 1"_s,
        "fin wait 1"_s + event<ack>[is_valid] = "fin wait 2"_s,
        "fin wait 2"_s + event<fin>[is_valid] / send_ack = "timed wait"_s,
        "timed wait"_s + event<timeout> = X);
  }
};

TEST(SML, SimpleFSM) {
  using namespace sml;

  sender s{};
  sm<tcp_release> sm{s};  // pass dependencies via ctor
  ASSERT_TRUE(sm.is("established"_s));

  sm.process_event(release{});  // complexity O(1)
  ASSERT_TRUE(sm.is("fin wait 1"_s));

  sm.process_event(ack{true});  // prints 'send: 0'
  ASSERT_TRUE(sm.is("fin wait 2"_s));

  sm.process_event(fin{42, true});  // prints 'send: 42'
  ASSERT_TRUE(sm.is("timed wait"_s));

  sm.process_event(timeout{});
  ASSERT_TRUE(sm.is(X));  // terminated
}
