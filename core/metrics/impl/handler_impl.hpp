/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_IMPL_HANDLER_IMPL_HPP
#define KAGOME_CORE_METRICS_IMPL_HANDLER_IMPL_HPP

#include <prometheus/collectable.h>
#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>
#include <memory>
#include <string_view>
#include "log/logger.hpp"
#include "metrics/handler.hpp"

namespace kagome::api {
  class Session;
}

namespace kagome::metrics {

  class HandlerImpl : public Handler {
   public:
    explicit HandlerImpl(prometheus::Registry &registry);
    ~HandlerImpl() override = default;

    void registerCollectable(
        const std::weak_ptr<prometheus::Collectable> &collectable);
    void removeCollectable(
        const std::weak_ptr<prometheus::Collectable> &collectable);

    void onSessionRequest(Session::Request request,
                          std::shared_ptr<Session> session) override;

   private:
    static void cleanupStalePointers(
        std::vector<std::weak_ptr<prometheus::Collectable>> &collectables);
    std::size_t writeResponse(std::shared_ptr<Session> session,
                              Session::Request request,
                              const std::string &body);

    std::mutex collectables_mutex_;
    std::vector<std::weak_ptr<prometheus::Collectable>> collectables_;
    prometheus::Family<prometheus::Counter> &bytes_transferred_family_;
    prometheus::Counter &bytes_transferred_;
    prometheus::Family<prometheus::Counter> &num_scrapes_family_;
    prometheus::Counter &num_scrapes_;
    prometheus::Family<prometheus::Summary> &request_latencies_family_;
    prometheus::Summary &request_latencies_;

    log::Logger logger_;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_IMPL_HANDLER_IMPL_HPP
