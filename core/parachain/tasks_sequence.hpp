//
// Created by iceseer on 11/20/22.
//

#ifndef KAGOME_TASKS_SEQUENCE_HPP
#define KAGOME_TASKS_SEQUENCE_HPP

#include <memory>
#include <type_traits>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>

namespace kagome::thread {

  // clang-format off
  /*
   * Code of `sequence` allows to execute multiple tasks in different threads
   * like so the output of the task is the incoming argument for the next one.
   * Additionally it makes check of the returned outcome::result<T>.
   *
   * ThreadQueueContext allows you to make wrapper around any sync/async subsystem.
   *
   * Example:
   *    tp_ - std::shared_ptr<kagome::thread::ThreadPool>
   *    c_ - std::shared_ptr<boost::asio::io_context>
   *
   *    thread::sequence(
   *        thread::createTask(tp_, []() -> outcome::result<int> { return 100; }),
   *        thread::createTask(
   *            c_, [](auto a) -> outcome::result<float> { return 10.f + a; }),
   *        thread::createTask(tp_,
   *                           [](auto b) -> outcome::result<std::string> {
   *                             static_assert(std::is_floating_point_v<decltype(b)>);
   *                             return std::to_string(static_cast<int>(b))
   *                                    + " is the result";
   *                           }),
   *        thread::createTask(tp_,
   *                           [](auto c) { assert(c == "110 is the result"); }));
   * */
  // clang-format on

  template <typename T>
  struct ThreadQueueContext {
    template <typename D>
    [[maybe_unused]] ThreadQueueContext(D &&);
    template <typename F>
    [[maybe_unused]] void operator()(F &&func);
  };

  template <>
  struct ThreadQueueContext<std::weak_ptr<boost::asio::io_context>> {
    using Type = std::weak_ptr<boost::asio::io_context>;
    Type t;

    template <typename D>
    ThreadQueueContext(D &&arg) : t{std::forward<D>(arg)} {}

    template <typename F>
    void operator()(F &&func) {
      if (auto call_context = t.lock()) {
        boost::asio::post(*call_context, std::forward<F>(func));
      }
    }
  };

  template <>
  struct ThreadQueueContext<std::shared_ptr<boost::asio::io_context>>
      : ThreadQueueContext<std::weak_ptr<boost::asio::io_context>> {
    template <typename D>
    ThreadQueueContext(D &&arg)
        : ThreadQueueContext<std::weak_ptr<boost::asio::io_context>>(
            std::forward<D>(arg)) {}
  };

  template <typename T>
  auto createThreadQueueContext(T &&t) {
    return ThreadQueueContext<std::decay_t<T>>{std::forward<T>(t)};
  }

  template <typename C, typename F>
  auto createTask(C &&c, F &&f) {
    return std::make_pair(createThreadQueueContext(std::forward<C>(c)),
                          std::forward<F>(f));
  }

  template <typename T, typename K, typename... Args>
  void sequence(std::pair<T, K> &&t, Args &&...args) {
    __internal_contextCall(std::move(t.first),
                           [func{std::move(t.second)},
                            forwarding_func{__internal_bindArgs(
                                std::forward<Args>(args)...)}]() mutable {
                             forwarding_func(func());
                           });
  }

  template <typename T, typename K>
  void sequence(std::pair<T, K> &&t) {
    __internal_contextCall(std::move(t.first), std::move(t.second));
  }

  template <typename T, typename F>
  void __internal_contextCall(ThreadQueueContext<T> &&t, F &&f) {
    t(std::forward<F>(f));
  }

  template <typename R, typename T, typename K>
  void __internal_executeI(R &&r, std::pair<T, K> &&t) {
    if (std::forward<R>(r).has_value()) {
      __internal_contextCall(
          std::move(t.first),
          [r{std::forward<R>(r).value()}, func{std::move(t.second)}]() mutable {
            func(std::move(r));
          });
    }
  }

  template <typename R, typename T, typename K, typename... Args>
  void __internal_executeI(R &&r, std::pair<T, K> &&t, Args &&...args) {
    if (std::forward<R>(r).has_value()) {
      __internal_contextCall(std::move(t.first),
                             [r{std::forward<R>(r).value()},
                              func{std::move(t.second)},
                              forwarding_func{__internal_bindArgs(
                                  std::forward<Args>(args)...)}]() mutable {
                               forwarding_func(func(std::move(r)));
                             });
    }
  }

  template <typename... Args>
  auto __internal_bindArgs(Args &&...args) {
    return std::bind(
        [](auto &&...args) mutable { __internal_executeI(std::move(args)...); },
        std::placeholders::_1,
        std::forward<Args>(args)...);
  }

}  // namespace kagome::thread

#endif  // KAGOME_TASKS_SEQUENCE_HPP
