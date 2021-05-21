/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "handler_impl.hpp"
#include <prometheus/text_serializer.h>
#include "log/logger.hpp"

using namespace prometheus;

std::vector<MetricFamily> CollectMetrics(
    const std::vector<std::weak_ptr<prometheus::Collectable>> &collectables) {
  auto collected_metrics = std::vector<MetricFamily>{};

  for (auto &&wcollectable : collectables) {
    auto collectable = wcollectable.lock();
    if (!collectable) {
      continue;
    }

    auto &&metrics = collectable->Collect();
    collected_metrics.insert(collected_metrics.end(),
                             std::make_move_iterator(metrics.begin()),
                             std::make_move_iterator(metrics.end()));
  }

  return collected_metrics;
}

namespace kagome::metrics {

  HandlerImpl::HandlerImpl(Registry &registry)
      : bytes_transferred_family_(
          BuildCounter()
              .Name("exposer_transferred_bytes_total")
              .Help("Transferred bytes to metrics services")
              .Register(registry)),
        bytes_transferred_(bytes_transferred_family_.Add({})),
        num_scrapes_family_(BuildCounter()
                                .Name("exposer_scrapes_total")
                                .Help("Number of times metrics were scraped")
                                .Register(registry)),
        num_scrapes_(num_scrapes_family_.Add({})),
        request_latencies_family_(
            BuildSummary()
                .Name("exposer_request_latencies")
                .Help("Latencies of serving scrape requests, in microseconds")
                .Register(registry)),
        request_latencies_(request_latencies_family_.Add(
            {}, Summary::Quantiles{{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}})),
        logger_{log::createLogger("HandlerImpl", "metrics")} {}

  void HandlerImpl::onSessionRequest(Session::Request request,
                                     std::shared_ptr<Session> session) {
    auto start_time_of_request = std::chrono::steady_clock::now();

    std::vector<MetricFamily> metrics;

    {
      std::lock_guard<std::mutex> lock{collectables_mutex_};
      metrics = CollectMetrics(collectables_);
    }

    const TextSerializer serializer;

    auto size = writeResponse(session, request, serializer.Serialize(metrics));

    auto stop_time_of_request = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        stop_time_of_request - start_time_of_request);
    request_latencies_.Observe(duration.count());

    bytes_transferred_.Increment(size);
    num_scrapes_.Increment();
  }

  std::size_t HandlerImpl::writeResponse(std::shared_ptr<Session> session,
                                         Session::Request request,
                                         const std::string &body) {
    Session::Response res{boost::beast::http::status::ok, request.version()};
    res.set(boost::beast::http::field::content_type,
            "text/plain; charset=utf-8");
    res.set(boost::beast::http::field::content_length, body.size());
    res.keep_alive(request.keep_alive());
    res.body() = body;
    res.prepare_payload();
    session->respond(res);
    return body.size();
  }

  void HandlerImpl::registerCollectable(
      const std::weak_ptr<Collectable> &collectable) {
    std::lock_guard<std::mutex> lock{collectables_mutex_};
    cleanupStalePointers(collectables_);
    collectables_.push_back(collectable);
  }

  void HandlerImpl::removeCollectable(
      const std::weak_ptr<Collectable> &collectable) {
    std::lock_guard<std::mutex> lock{collectables_mutex_};

    auto locked = collectable.lock();
    auto same_pointer = [&locked](const std::weak_ptr<Collectable> &candidate) {
      return locked == candidate.lock();
    };

    collectables_.erase(
        std::remove_if(
            std::begin(collectables_), std::end(collectables_), same_pointer),
        std::end(collectables_));
  }

  void HandlerImpl::cleanupStalePointers(
      std::vector<std::weak_ptr<Collectable>> &collectables) {
    collectables.erase(
        std::remove_if(std::begin(collectables),
                       std::end(collectables),
                       [](const std::weak_ptr<Collectable> &candidate) {
                         return candidate.expired();
                       }),
        std::end(collectables));
  }

}  // namespace kagome::metrics
