/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "aio/timer.hpp"

#include <atomic>
#include <libp2p/basic/scheduler.hpp>
#include <optional>

#include "common/spin_lock.hpp"
#include "utils/safe_object.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace kagome::aio {
  /**
   * Destroys `libp2p::basic::Scheduler::Handle` on libp2p thread.
   */
  class TimerImplCancel : public CancelDtor {
   public:
    struct State {
      ~State();

      std::atomic_bool cancelled = false;
      SafeObject<std::optional<libp2p::basic::Scheduler::Handle>,
                 common::spin_lock>
          handle;
    };

    TimerImplCancel(std::weak_ptr<boost::asio::io_context> io,
                    std::shared_ptr<State> state);

    ~TimerImplCancel() override;

   private:
    std::weak_ptr<boost::asio::io_context> io_;
    std::shared_ptr<State> state_;
  };

  /**
   * Uses `libp2p::basic::Scheduler`.
   */
  class TimerImpl : public Timer {
   public:
    TimerImpl(std::weak_ptr<boost::asio::io_context> io,
              std::weak_ptr<libp2p::basic::Scheduler> scheduler);

    void timer(Cb, Delay) override;
    Cancel timerCancel(Cb, Delay) override;

   private:
    std::weak_ptr<boost::asio::io_context> io_;
    std::weak_ptr<libp2p::basic::Scheduler> scheduler_;
  };
}  // namespace kagome::aio
