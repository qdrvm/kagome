/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_IMPL_EXPOSER_IMPL_HPP
#define KAGOME_CORE_METRICS_IMPL_EXPOSER_IMPL_HPP

#include <thread>

#include "application/app_state_manager.hpp"
#include "log/logger.hpp"
#include "metrics/exposer.hpp"

namespace kagome::metrics {

  class ExposerImpl : public Exposer,
                      public std::enable_shared_from_this<ExposerImpl> {
    log::Logger logger_;

   public:
    ExposerImpl(
        const std::shared_ptr<application::AppStateManager> &app_state_manager,
        Exposer::Configuration exposer_config,
        Session::Configuration session_config);

    ~ExposerImpl() override = default;

    bool prepare() override;
    bool start() override;
    void stop() override;

   private:
    void acceptOnce();

    std::shared_ptr<Context> context_;
    const Configuration config_;
    const Session::Configuration session_config_;

    std::unique_ptr<Acceptor> acceptor_;

    std::shared_ptr<Session> new_session_;

    std::shared_ptr<std::thread> thread_;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_IMPL_EXPOSER_IMPL_HPP
