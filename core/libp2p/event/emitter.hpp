/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EMITTER_HPP
#define KAGOME_EMITTER_HPP

#include <functional>

#include <boost/signals2.hpp>

namespace libp2p::event {

/**
 * Make an event emitter with a specified type as an event argument
 * @param Tag - type (should be a structure) to be emitted by this instance
 * @note usage example can be found in event_emitter_test.cpp
 */
#define KAGOME_EMITS(Tag)                             \
 private:                                             \
  boost::signals2::signal<void(Tag)> signal_##Tag##_; \
                                                      \
 public:                                              \
  void on(const std::function<void(Tag)> &handler) {  \
    signal_##Tag##_.connect(handler);                 \
  }                                                   \
                                                      \
  void emit(Tag tag) {                                \
    signal_##Tag##_(std::move(tag));                  \
  }

}  // namespace libp2p::event

#endif  // KAGOME_EMITTER_HPP
